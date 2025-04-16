#pragma once

#include <maya/MPxNode.h>
#include <maya/MFnTypedAttribute.h>

class GroupDataNode : public MPxNode
{
public:
    inline static MTypeId typeId = 0x80001;
    inline static MString typeName = "uvGroupData";

    inline static char kAttributeName[] = "uvShellGroupData";
    inline static char kAttribueShortName[] = "usgd";

    inline static MObject uvGroupData;

    static void* creator();
    static MStatus initialize();

    GroupDataNode();
    virtual ~GroupDataNode();
};
