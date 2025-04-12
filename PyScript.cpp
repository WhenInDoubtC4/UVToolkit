#include "PyScript.h"

PyScript::PyScript(const QString& resourcePath)
    : _resourcePath(resourcePath)
{
    QFile file(resourcePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        std::cout << "Failed to open file " << resourcePath.toStdString() << "\n";
        return;
    }

    if (!file.fileName().endsWith(".py"))
    {
        std::cout  << "File " << resourcePath.toStdString() << " is not a python script. This might cause issues later on\n";
    }

    QTextStream inStream(&file);
    _data = inStream.readAll();
}

MString PyScript::getMString() const
{
    QByteArray byteArray(_data.toUtf8());
    return MString(byteArray.data());
}

void PyScript::setGlobal(QString name, QVariant value)
{
    //This regex took 2 of us 3 hours to build. We should never touch it again.
    QRegularExpression regex(QStringLiteral(R"(^%1\s*(?::[^=\r\n]*)?(.*)$)").arg(name), QRegularExpression::PatternOption::MultilineOption);
    QRegularExpressionMatch match = regex.match(_data);

    if (!match.hasMatch())
    {
        std::cout << "Cannot find global variable " << name.toStdString() << " in script " << _resourcePath.toStdString() << "\n";
        return;
    }

    std::cout << "Value type name is " << value.typeName() << "\n";

    //Bools get special treatment because python is stupid
    if (strcmp(value.typeName(), "bool") == 0)
    {
        _data.replace(match.capturedStart(1), match.capturedLength(1), value.toBool() ? "= True" : "= False");
    }
    //Strings will need quotation marks
    //TODO: Handle std strings?
    else if (strcmp(value.typeName(), "QString") == 0)
    {
        _data.replace(match.capturedStart(1), match.capturedLength(1), QStringLiteral("= \"%1\"").arg(value.toString()));
    }
    else
    {
        _data.replace(match.capturedStart(1), match.capturedLength(1), QStringLiteral("= %1").arg(value.toString()));
    }

    std::cout << "Replaced script:\n" << _data.toStdString() << "\n";
}
