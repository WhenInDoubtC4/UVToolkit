#include "GroupDataNode.h"

GroupDataNode::GroupDataNode()
    : MPxNode()
{

}

GroupDataNode::~GroupDataNode()
{

}

void* GroupDataNode::creator()
{
    return new GroupDataNode();
}

MStatus GroupDataNode::initialize()
{
    MFnTypedAttribute attr;
    MObject attrObj = attr.create(kAttributeName, kAttribueShortName, MFnData::kString);

    attr.setStorable(true);
    attr.setKeyable(false);
    attr.setHidden(false);

    addAttribute(attrObj);

    uvGroupData = attrObj;

    return MStatus::kSuccess;
}
