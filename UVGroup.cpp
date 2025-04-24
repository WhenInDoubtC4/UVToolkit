#include "UVGroup.h"

UVGroup::UVGroup() {}

QJsonObject UVGroup::serialize()
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
    //Do this because of the root
    result["parent"] = _parent ? static_cast<qint64>(_parent->_id) : -1;

    QJsonArray childArray;
    for (UVGroup* childGroup : _children)
    {
        childArray << childGroup->serialize();
    }

    result["children"] = childArray;

    return result;
}

UVGroup::AABB UVGroup::getAABB() const
{
    AABB result;

    for (const QPair<MDagPath, unsigned int>& shell : _shells)
    {
        MObject faces = MeshData::getMeshData(shell.first)->getUvShell(shell.second).faces;
        for (MItMeshPolygon it(shell.first, faces); !it.isDone(); it.next())
        {
            for (unsigned int v = 0; v < it.polygonVertexCount(); v++)
            {
                float uvPoint[2];
                it.getUV(v, uvPoint);

                if (uvPoint[0] < result.xmin) result.xmin = uvPoint[0];
                if (uvPoint[0] > result.xmax) result.xmax = uvPoint[0];

                if (uvPoint[1] < result.ymin) result.ymin = uvPoint[1];
                if (uvPoint[1] > result.ymax) result.ymax = uvPoint[1];
            }
        }
    }

    return result;
}

double UVGroup::getUvArea()
{
    double result = -1.;

    for (const QPair<MDagPath, unsigned int>& shell : _shells)
    {
        MeshData::UVData shellData = MeshData::getMeshData(shell.first)->getUvShell(shell.second);
        for (MItMeshPolygon it(shell.first, shellData.faces); !it.isDone(); it.next())
        {
            if (it.zeroUVArea()) continue;

            it.getUVArea(result);
            return result;
        }
    }

    return result;
}

void UVGroup::layout()
{
    qDebug() << "Group layout exec";

    //TODO: Layout the group and its contents ONLY
}
