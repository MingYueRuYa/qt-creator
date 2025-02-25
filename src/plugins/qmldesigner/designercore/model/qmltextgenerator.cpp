// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "qmltextgenerator.h"

#include <QVariant>
#include <QColor>
#include <QVector4D>
#include <QVector3D>
#include <QVector2D>

#include "bindingproperty.h"
#include "signalhandlerproperty.h"
#include "nodeproperty.h"
#include "nodelistproperty.h"
#include "variantproperty.h"
#include <nodemetainfo.h>
#include "model.h"

using namespace QmlDesigner;
using namespace QmlDesigner::Internal;

inline static QString properColorName(const QColor &color)
{
    if (color.alpha() == 255)
        return QString::asprintf("#%02x%02x%02x", color.red(), color.green(), color.blue());
    else
        return QString::asprintf("#%02x%02x%02x%02x", color.alpha(), color.red(), color.green(), color.blue());
}

inline static QString doubleToString(const PropertyName &propertyName, double d)
{
    static QVector<PropertyName> lowPrecisionProperties
        = {"width", "height", "x", "y", "rotation", "scale", "opacity"};
    int precision = 5;
    if (propertyName.contains("anchors") || propertyName.contains("font")
        || lowPrecisionProperties.contains(propertyName))
        precision = 3;

    QString string = QString::number(d, 'f', precision);
    if (string.contains(QLatin1Char('.'))) {
        while (string.at(string.length()- 1) == QLatin1Char('0'))
            string.chop(1);
        if (string.at(string.length()- 1) == QLatin1Char('.'))
            string.chop(1);
    }
    return string;
}

static QString unicodeEscape(const QString &stringValue)
{
    if (stringValue.count() == 1) {
        ushort code = stringValue.at(0).unicode();
        bool isUnicode = code <= 127;
        if (isUnicode) {
            return stringValue;
        } else {
            QString escaped;
            escaped += "\\u";
            escaped += QString::number(code, 16).rightJustified(4, '0');
            return escaped;
        }
    }
    return stringValue;
}

QmlTextGenerator::QmlTextGenerator(const PropertyNameList &propertyOrder,
                                   const TextEditor::TabSettings &tabSettings,
                                   const int startIndentDepth)
    : m_propertyOrder(propertyOrder)
    , m_tabSettings(tabSettings)
    , m_startIndentDepth(startIndentDepth)
{
}

QString QmlTextGenerator::toQml(const AbstractProperty &property, int indentDepth) const
{
    if (property.isBindingProperty()) {
        return property.toBindingProperty().expression();
    } else if (property.isSignalHandlerProperty()) {
        return property.toSignalHandlerProperty().source();
    } else if (property.isSignalDeclarationProperty()) {
        return property.toSignalDeclarationProperty().signature();
    } else if (property.isNodeProperty()) {
        return toQml(property.toNodeProperty().modelNode(), indentDepth);
    } else if (property.isNodeListProperty()) {
        const QList<ModelNode> nodes = property.toNodeListProperty().toModelNodeList();
        if (property.isDefaultProperty()) {
            QString result;
            for (int i = 0; i < nodes.length(); ++i) {
                if (i > 0)
                    result += QStringLiteral("\n\n");
                result += m_tabSettings.indentationString(0, indentDepth, 0);
                result += toQml(nodes.at(i), indentDepth);
            }
            return result;
        } else {
            QString result = QStringLiteral("[");
            const int arrayContentDepth = indentDepth + m_tabSettings.m_indentSize;
            const QString arrayContentIndentation(arrayContentDepth, QLatin1Char(' '));
            for (int i = 0; i < nodes.length(); ++i) {
                if (i > 0)
                    result += QLatin1Char(',');
                result += QLatin1Char('\n');
                result += arrayContentIndentation;
                result += toQml(nodes.at(i), arrayContentDepth);
            }
            return result + QLatin1Char(']');
        }
    } else if (property.isVariantProperty()) {
        const VariantProperty variantProperty = property.toVariantProperty();
        const QVariant value = variantProperty.value();
        const QString stringValue = value.toString();

        if (property.name() == "id")
            return stringValue;

          if (false) {
          }
        if (variantProperty.holdsEnumeration()) {
            return variantProperty.enumeration().toString();
        } else {

            switch (value.userType()) {
            case QMetaType::Bool:
                if (value.toBool())
                    return QStringLiteral("true");
                else
                    return QStringLiteral("false");

            case QMetaType::QColor:
                return QStringLiteral("\"%1\"").arg(properColorName(value.value<QColor>()));

            case QMetaType::Float:
            case QMetaType::Double:
                return doubleToString(property.name(), value.toDouble());
            case QMetaType::Int:
            case QMetaType::LongLong:
            case QMetaType::UInt:
            case QMetaType::ULongLong:
                return stringValue;
            case QMetaType::QString:
            case QMetaType::QChar:
                return QStringLiteral("\"%1\"").arg(escape(unicodeEscape(stringValue)));
            case QMetaType::QVector2D: {
                auto vec = value.value<QVector2D>();
                return QStringLiteral("Qt.vector2d(%1, %2)").arg(vec.x()).arg(vec.y());
            }
            case QMetaType::QVector3D: {
                auto vec = value.value<QVector3D>();
                return QStringLiteral("Qt.vector3d(%1, %2, %3)").arg(vec.x()).arg(vec.y()).arg(vec.z());
            }
            case QMetaType::QVector4D: {
                auto vec = value.value<QVector4D>();
                return QStringLiteral("Qt.vector4d(%1, %2, %3, %4)")
                    .arg(vec.x())
                    .arg(vec.y())
                    .arg(vec.z())
                    .arg(vec.w());
            }
            default:
                return QStringLiteral("\"%1\"").arg(escape(stringValue));
            }
        }
    } else {
        Q_ASSERT("Unknown property type");
        return QString();
    }
}

QString QmlTextGenerator::toQml(const ModelNode &node, int indentDepth) const
{
    QString type = QString::fromLatin1(node.type());
    QString url;
    if (type.contains('.')) {
        QStringList nameComponents = type.split('.');
        url = nameComponents.constFirst();
        type = nameComponents.constLast();
    }

    QString alias;
    if (!url.isEmpty()) {
        for (const Import &import : node.model()->imports()) {
            if (import.url() == url) {
                alias = import.alias();
                break;
            }
            if (import.file() == url) {
                alias = import.alias();
                break;
            }
        }
    }

    QString result;

    if (!alias.isEmpty())
        result = alias + '.';

    result += type;
    if (!node.behaviorPropertyName().isEmpty()) {
        result += " on " +  node.behaviorPropertyName();
    }

    result += QStringLiteral(" {\n");

    const int propertyIndentDepth = indentDepth + m_tabSettings.m_indentSize;

    const QString properties = propertiesToQml(node, propertyIndentDepth);

    return result + properties + m_tabSettings.indentationString(0, indentDepth, 0)
           + QLatin1Char('}');
}

QString QmlTextGenerator::propertiesToQml(const ModelNode &node, int indentDepth) const
{
    QString topPart;
    QString bottomPart;

    PropertyNameList nodePropertyNames = node.propertyNames();
    bool addToTop = true;

    for (const PropertyName &propertyName : std::as_const(m_propertyOrder)) {
        if (propertyName == "id") {
            // the model handles the id property special, so:
            if (!node.id().isEmpty()) {
                QString idLine = m_tabSettings.indentationString(0, indentDepth, 0);
                idLine += QStringLiteral("id: ");
                idLine += node.id();
                idLine += QLatin1Char('\n');

                if (addToTop)
                    topPart.append(idLine);
                else
                    bottomPart.append(idLine);
            }
        } else if (propertyName.isEmpty()) {
            addToTop = false;
        } else if (nodePropertyNames.removeAll(propertyName)) {
            const QString newContent = propertyToQml(node.property(propertyName), indentDepth);

            if (addToTop)
                topPart.append(newContent);
            else
                bottomPart.append(newContent);
        }
    }

    for (const PropertyName &propertyName : std::as_const(nodePropertyNames)) {
        bottomPart.prepend(propertyToQml(node.property(propertyName), indentDepth));
    }

    return topPart + bottomPart;
}

QString QmlTextGenerator::propertyToQml(const AbstractProperty &property, int indentDepth) const
{
    QString result;

    if (property.isDefaultProperty()) {
        result = toQml(property, indentDepth);
    } else {
        if (property.isDynamic()) {
            result = m_tabSettings.indentationString(0, indentDepth, 0)
                    + QStringLiteral("property ")
                    + QString::fromUtf8(property.dynamicTypeName())
                    + QStringLiteral(" ")
                    + QString::fromUtf8(property.name())
                    + QStringLiteral(": ")
                    + toQml(property, indentDepth);
        }
        if (property.isSignalDeclarationProperty()) {
            result = m_tabSettings.indentationString(0, indentDepth, 0) + "signal" + " "
                     + QString::fromUtf8(property.name()) + " " + toQml(property, indentDepth);
        } else {
            result = m_tabSettings.indentationString(0, indentDepth, 0)
                    + QString::fromUtf8(property.name())
                    + QStringLiteral(": ")
                    + toQml(property, indentDepth);
        }
    }

    result += QLatin1Char('\n');

    return result;
}

QString QmlTextGenerator::escape(const QString &value)
{
    QString result = value;

    if (value.count() == 6 && value.startsWith("\\u")) //Do not dobule escape unicode chars
        return result;

    result.replace(QStringLiteral("\\"), QStringLiteral("\\\\"));

    result.replace(QStringLiteral("\""), QStringLiteral("\\\""));
    result.replace(QStringLiteral("\t"), QStringLiteral("\\t"));
    result.replace(QStringLiteral("\r"), QStringLiteral("\\r"));
    result.replace(QStringLiteral("\n"), QStringLiteral("\\n"));

    return result;
}
