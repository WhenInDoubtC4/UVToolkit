#pragma once

#include <QString>
#include <QFile>
#include <QTextStream>
#include <QVariant>
#include <QRegularExpression>
#include <QDebug>

#include <maya/MString.h>
#include <iostream>

class PyScript
{
public:
    PyScript(const QString& resourcePath);

    MString getMString() const;

    void setGlobal(QString name, QVariant value);

private:
    QString _resourcePath;
    QString _data;
};
