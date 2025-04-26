#include "MeshManager.h"

MeshManager::MeshManager()
    : QObject()
{
    qDebug() << "Creating mesh manager";

    _groupData = getGroupDataNode();
    gatherExistingMeshes();
    _groupDataRoot = new UVGroup();
    _groupDataRoot->_id = -1;
    _groupDataRoot->_name = "<ROOT>";
}

MeshManager::~MeshManager()
{
    qDebug() << "Deleting mesh manager";

    //Remove the group data node
    //TODO: This might have to be disabled so that it gets stored?
    MGlobal::deleteNode(_groupData);

    for (MeshData* data : MeshData::_meshData) delete data;

    if (_groupDataRoot) clearGroupData(_groupDataRoot);
}

MeshManager* MeshManager::getInst()
{
    if (_instance) return _instance;

    init();
    return _instance;
}

void MeshManager::init()
{
    if (_instance) return;

    _instance = new MeshManager();
}


void MeshManager::cleanup()
{
    if (!_instance) return;

    _instance->~MeshManager();
}

void MeshManager::addMesh(MeshData* mesh)
{
    QObject::connect(mesh, &MeshData::uvShellAdded, this, &MeshManager::onUvShellAdded);
    QObject::connect(mesh, &MeshData::uvShellIndexChanged, this, &MeshManager::onUvShellIndexChanged);
    QObject::connect(mesh, &MeshData::uvShellRemoved, this, &MeshManager::onUvShellRemoved);
    QObject::connect(mesh, &MeshData::uvShellSplit, this, &MeshManager::onUvShellSplit);
    mesh->initUvShells();
    QObject::connect(mesh, &MeshData::uvDataRefreshed, this, &MeshManager::onUvDataRefreshed);
    MeshData::_meshData << mesh;
}

MeshData* MeshManager::removeMeshByName(const MString& name)
{
    MeshData* meshToRemove = nullptr;
    for (MeshData* data : MeshData::_meshData)
    {
        if (data->getMeshName() != name) continue;

        //This will only remove one mesh at a time which can potentially be bad. Unsure if it will ever trigger multiple removals though
        meshToRemove = data;
        break;
    }

    if (!meshToRemove) return nullptr;
    MeshData::_meshData.remove(meshToRemove);

    return meshToRemove;
}

UVGroup* MeshManager::createGroup(UVGroup* parent)
{
    if (!parent) parent = _groupDataRoot;

    auto result = new UVGroup();
    result->_id = ++_nextGroupId;
    result->_parent = parent;

    parent->_children << result;

    emit groupCreated(result);

    return result;
}

UVGroup* MeshManager::createGroupFromJsonRecursive(const QJsonObject& jsonObject)
{
    auto result = new UVGroup();

    result->_id = jsonObject["id"].toInteger();

    //Make sure groups created after will have proper IDs
    _nextGroupId = std::max(_nextGroupId, result->_id);

    result->_name = jsonObject["name"].toString();

    QJsonArray shellArray = jsonObject["shells"].toArray();
    for (const QJsonValue& shellValue : shellArray)
    {
        QJsonObject shell = shellValue.toObject();

        QString dagPathName = shell["path"].toString();
        unsigned int shellIndex = shell["shell"].toInteger();

        MSelectionList meshSelection;
        meshSelection.add(MQtUtil::toMString(dagPathName));

        MDagPath path;
        if (!meshSelection.getDagPath(0, path))
        {
            qDebug() << "Failed to deserialize path name" << dagPathName;
            continue;
        }

        qDebug() << "Deserialized shell" << path.fullPathName().asChar() << shellIndex << "for group" << result->_id;
        result->_shells << qMakePair(path, shellIndex);
    }

    long long parentIndex = jsonObject["parent"].toInteger();

    UVGroup* parentGroup = findGroupWithId(parentIndex, _groupDataRoot);
    if (!parentGroup)
    {
        qDebug() << "Could not find parent group with index" << parentIndex;
    }
    result->_parent = parentGroup;

    //The root item
    if (parentIndex < 0)
    {
        _groupDataRoot = result;
    }

    QJsonArray childArray = jsonObject["children"].toArray();
    for (const QJsonValue& childValue : childArray)
    {
        QJsonObject childObject = childValue.toObject();
        UVGroup* childGroup = createGroupFromJsonRecursive(childObject);
        if (childGroup) result->_children << childGroup;
    }

    emit groupCreated(result);

    return result;
}

QList<UVGroup*> MeshManager::getTopLevelGroups()
{
    QList<UVGroup*> result;

    for (UVGroup* topLevelGroup : _groupDataRoot->_children)
    {
        result << topLevelGroup;
    }

    return result;
}

void MeshManager::deleteGroup(UVGroup* group)
{
    group->_parent->_children.remove(group);

    for (UVGroup* childGroup : group->_children) group->_parent->_children << childGroup;

    delete group;
}

void MeshManager::addUntrackedMeshes()
{
    for(MItDag it(MItDag::TraversalType::kBreadthFirst, MFn::kTransform); !it.isDone(); it.next())
    {
        //Filter meshes
        MDagPath path;
        if (it.getPath(path) != MStatus::kSuccess)
        {
            MGlobal::displayError("Cannot get dag path for " + it.fullPathName());
            continue;
        }
        if (!path.hasFn(MFn::kMesh)) continue;

        //Create top level item for the mesh
        MFnMesh mesh(path);

        bool isTracked = false;
        for (MeshData* data : MeshData::_meshData)
        {
            if (data->getDagPath().fullPathName() == mesh.dagPath().fullPathName())
            {
                isTracked = true;
                break;
            }
        }

        if (isTracked) continue;

        addMesh(new MeshData(path));
    }
}

void MeshManager::readdExistingGroups()
{
    qDebug() << "Readding existing groups";

    MeshData::removeInvalidMeshes();
    readdExistingGroups_impl(_groupDataRoot);
}

void UVGroup::addUvShell(MeshData* mesh, unsigned int shellIndex)
{
    _shells << qMakePair(mesh->getDagPath(), shellIndex);

    emit MeshManager::getInst()->uvShellAddedToGroup(this, mesh, shellIndex);
}

void UVGroup::removeUvShell(MeshData* mesh, unsigned int shellIndex)
{
    _shells.removeAll(qMakePair(mesh->getDagPath(), shellIndex));

    emit MeshManager::getInst()->uvShellRemovedFromGroup(this, mesh, shellIndex);
}

QJsonDocument MeshManager::serializeGroupData()
{
    QJsonObject rootObject = _groupDataRoot->serialize();
    return QJsonDocument(rootObject);
}

UVGroup* MeshManager::getShellGroup(MeshData* mesh, unsigned int shellindex) const
{
    return getShellGroup_impl(mesh, shellindex, _groupDataRoot);
}

void MeshManager::removeInvalidUvShells(MeshData* mesh)
{
    removeInvalidUvShells_impl(mesh, _groupDataRoot);
}

MObject MeshManager::getGroupDataNode()
{
    for (MItDependencyNodes it(MFn::kPluginDependNode); !it.isDone(); it.next())
    {
        MFnDependencyNode node(it.thisNode());
        if (node.typeName() != GroupDataNode::typeName) continue;

        return it.thisNode();
    }

    qDebug() << "Creating group data node";
    MObject newNode;

    //Create the ndde
    MDGModifier dgModifier;
    newNode = dgModifier.createNode(GroupDataNode::typeName);
    dgModifier.doIt();

    MFnDependencyNode depNode(newNode);
    depNode.setName("UVGroupData#");

    return newNode;
}

UVGroup* MeshManager::findGroupWithId(long long id, UVGroup* root)
{
    if (root->_id) return root;

    for (UVGroup* childGroup : root->_children)
    {
        findGroupWithId(id, childGroup);
    }

    return nullptr;
}

void MeshManager::gatherExistingMeshes()
{
    //Get meshes already in the scene when the object is loaded
    for(MItDag it(MItDag::TraversalType::kBreadthFirst, MFn::kTransform); !it.isDone(); it.next())
    {
        //Filter meshes
        MDagPath path;
        if (it.getPath(path) != MStatus::kSuccess)
        {
            MGlobal::displayError("Cannot get dag path for " + it.fullPathName());
            continue;
        }
        if (!path.hasFn(MFn::kMesh)) continue;

        //Create top level item for the mesh
        MFnMesh mesh(path);

        addMesh(new MeshData(path));
    }
}

void MeshManager::clearGroupData(UVGroup* root)
{
    for (UVGroup* childItem : root->_children)
    {
        clearGroupData(childItem);
    }
    root->_children.clear();
    delete root;
}

UVGroup* MeshManager::getShellGroup_impl(MeshData* mesh, unsigned int shellindex, UVGroup* root) const
{
    //Traverse the tree until the shell is found
    for (const QPair<MDagPath, unsigned int>& shell : root->_shells)
    {
        if (shell.first == mesh->getDagPath() && shell.second == shellindex) return root;
    }

    for (UVGroup* child : root->_children)
    {
        UVGroup* shellGroup = getShellGroup_impl(mesh, shellindex, child);
        if (shellGroup) return shellGroup;
    }

    return nullptr;
}

void MeshManager::removeInvalidUvShells_impl(MeshData* mesh, UVGroup* root)
{
    //Traverse the tree and get rid of any invalid shells
    QSet<unsigned int> invalidShells;
    for (const QPair<MDagPath, unsigned int>& shell : root->_shells)
    {
        if (shell.second >= mesh->getNumUvShells())
        {
            invalidShells << shell.second;
        }
    }

    for (const unsigned int& invalidShellIndex : invalidShells)
    {
        root->removeUvShell(mesh, invalidShellIndex);

        emit meshUvShellRemoved(mesh, invalidShellIndex);
    }

    for (UVGroup* child : root->_children)
    {
        removeInvalidUvShells_impl(mesh, child);
    }
}

void MeshManager::readdExistingGroups_impl(UVGroup* root)
{
    if (root->_id >= 0) emit groupCreated(root);

    for (UVGroup* child : root->_children)
    {
        readdExistingGroups_impl(child);
    }
}

void MeshManager::onUvShellAdded(MeshData* mesh, unsigned int shellIndex)
{
    emit meshUvShellAdded(mesh, shellIndex);
}

void MeshManager::onUvShellIndexChanged(MeshData* mesh, unsigned int oldIndex, unsigned int newIndex)
{
    emit meshUvShellIndexChanged(mesh, oldIndex, newIndex);
}

void MeshManager::onUvShellRemoved(MeshData* mesh, unsigned int index)
{
    //Remove shell from the group it belonged to, if applicable
    UVGroup* shellGroup = getShellGroup(mesh, index);
    if (shellGroup) shellGroup->removeUvShell(mesh, index);

    emit meshUvShellRemoved(mesh, index);
}

void MeshManager::onUvShellSplit(MeshData* mesh, unsigned int oldShell, const QSet<int>& newIndices)
{
    UVGroup* originalGroup = getShellGroup(mesh, oldShell);

    if (!originalGroup) return;

    for (const int& newShellIndex : newIndices)
    {
        originalGroup->addUvShell(mesh, newShellIndex);
    }
}

void MeshManager::onUvDataRefreshed(MeshData* mesh)
{
    //Remove invalid UV shells from groups
    removeInvalidUvShells(mesh);

    emit meshUvDataRefreshed(mesh);
}


