#include "MeshManager.h"

MeshManager::MeshManager()
    : QObject()
{
    qDebug() << "Creating mesh manager";

    _groupData = getGroupDataNode();
    gatherExistingMeshes();
    _groupDataRoot = new UVGroup();
    _groupDataRoot->_name = "<ROOT>";
}

MeshManager::~MeshManager()
{
    qDebug() << "Deleting mesh manager";

    //Remove the group data node
    //TODO: This might have to be disabled so that it gets stored?
    MGlobal::deleteNode(_groupData);

    for (MeshData* data : _meshData) delete data;

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
    mesh->initUvShells();
    _meshData << mesh;
}

MeshData* MeshManager::removeMeshByName(const MString& name)
{
    MeshData* meshToRemove = nullptr;
    for (MeshData* data : _meshData)
    {
        if (data->getMeshName() != name) continue;

        //This will only remove one mesh at a time which can potentially be bad. Unsure if it will ever trigger multiple removals though
        meshToRemove = data;
        break;
    }

    if (!meshToRemove) return nullptr;
    _meshData.remove(meshToRemove);

    return meshToRemove;
}

MeshManager::UVGroup* MeshManager::createGroup(UVGroup* parent)
{
    if (!parent) parent = _groupDataRoot;

    auto result = new UVGroup;
    result->_id = ++_nextGroupId;
    result->_parent = parent;

    parent->_children << result;

    emit groupCreated(result);

    return result;
}

void MeshManager::deleteGroup(UVGroup* group)
{
    group->_parent->_children.remove(group);

    for (UVGroup* childGroup : group->_children) group->_parent->_children << childGroup;

    delete group;
}

void MeshManager::UVGroup::addUvShell(MeshData* mesh, unsigned int shellIndex)
{
    _shells << qMakePair(mesh->getDagPath(), shellIndex);

    emit MeshManager::getInst()->uvShellAddedToGroup(this, mesh, shellIndex);
}

void MeshManager::UVGroup::removeUvShell(MeshData* mesh, unsigned int shellIndex)
{
    _shells.removeAll(qMakePair(mesh->getDagPath(), shellIndex));

    emit MeshManager::getInst()->uvShellRemovedFromGroup(this, mesh, shellIndex);
}

QJsonDocument MeshManager::serializeGroupData()
{
    QJsonObject rootObject = _groupDataRoot->serialize();
    return QJsonDocument(rootObject);
}

QJsonObject MeshManager::UVGroup::serialize()
{
    QJsonObject result;

    result["id"] = static_cast<qint64>(_id);
    result["name"] = _name;

    QJsonArray shellArray;
    for (const QPair<MDagPath, unsigned int>& shell : _shells)
    {
        QJsonObject shellObject;
        shellObject["path"] = shell.first.fullPathName().asChar();
        shellObject["shell"] = static_cast<qint64>(shell.second);

        shellArray << shellObject;
    }

    result["shells"] = shellArray;
    result["parent"] = static_cast<qint64>(_parent->_id);

    QJsonArray childArray;
    for (UVGroup* childGroup : _children)
    {
        childArray << childGroup->serialize();
    }

    result["children"] = childArray;

    return result;
}

MeshManager::UVGroup* MeshManager::getShellGroup(MeshData* mesh, unsigned int shellindex) const
{
    return getShellGroup_impl(mesh, shellindex, _groupDataRoot);
}

MObject MeshManager::getGroupDataNode()
{
    MObject result;

    //Find the group data node or create it
    MSelectionList selList;
    selList.add(GroupDataNode::typeName);

    //Node already exists
    if (!selList.isEmpty())
    {
        qDebug() << "Group data node already exists";
        selList.getDependNode(0, result);
        return result;
    }

    qDebug() << "Creating group data node";

    //Create the ndde
    MDGModifier dgModifier;
    result = dgModifier.createNode(GroupDataNode::typeName);
    dgModifier.doIt();

    MFnDependencyNode depNode(result);
    depNode.setName("UVGroupData#");

    return result;
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

MeshManager::UVGroup* MeshManager::getShellGroup_impl(MeshData* mesh, unsigned int shellindex, UVGroup* root) const
{
    //Traverse the tree until the shell is found
    for (const QPair<MDagPath, unsigned int>& shell : root->_shells)
    {
        if (shell.first == mesh->getDagPath() && shell.second == shellindex) return root;
    }

    for (UVGroup* child : root->_children)
    {
        return getShellGroup_impl(mesh, shellindex, child);
    }

    return nullptr;
}

void MeshManager::onUvShellAdded(MeshData* mesh, unsigned int shellIndex)
{
    emit meshUvShellAdded(mesh, shellIndex);
}
