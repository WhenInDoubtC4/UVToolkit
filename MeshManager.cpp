#include "MeshManager.h"

MeshManager::MeshManager()
    : QObject()
{
    qDebug() << "Creating mesh manager";

    _groupData = getGroupDataNode();
}

MeshManager::~MeshManager()
{
    qDebug() << "Deleting mesh manager";

    //Remove the group data node
    //TODO: This might have to be disabled so that it gets stored?
    MGlobal::deleteNode(_groupData);
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
