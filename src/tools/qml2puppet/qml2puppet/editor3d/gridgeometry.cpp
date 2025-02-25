// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

#ifdef QUICK3D_MODULE

#include "gridgeometry.h"

namespace QmlDesigner {
namespace Internal {

GridGeometry::GridGeometry()
    : GeometryBase()
{
    updateGeometry();
}

GridGeometry::~GridGeometry()
{
}

int GridGeometry::lines() const
{
    return m_lines;
}

float GridGeometry::step() const
{
    return m_step;
}

bool GridGeometry::isCenterLine() const
{
    return m_isCenterLine;
}

// Number of lines on each side of the center lines.
// These lines are not drawn if m_isCenterLine is true; lines and step are simply used to calculate
// the length of the center line in that case.
void GridGeometry::setLines(int count)
{
    count = qMax(count, 1);
    if (m_lines == count)
        return;
    m_lines = qMax(count, 1);
    emit linesChanged();
    updateGeometry();
}

// Space between lines
void GridGeometry::setStep(float step)
{
    step = qMax(step, 0.0f);
    if (qFuzzyCompare(m_step, step))
        return;
    m_step = step;
    emit stepChanged();
    updateGeometry();
}

void GridGeometry::setIsCenterLine(bool enabled)
{
    if (m_isCenterLine == enabled)
        return;

    m_isCenterLine = enabled;
    emit isCenterLineChanged();
    updateGeometry();
}

void GridGeometry::doUpdateGeometry()
{
    GeometryBase::doUpdateGeometry();

    QByteArray vertexData;
    fillVertexData(vertexData);

    setVertexData(vertexData);

    int lastIndex = (vertexData.size() - 1) / int(sizeof(QVector3D));
    auto vertexPtr = reinterpret_cast<QVector3D *>(vertexData.data());
    setBounds(QVector3D(vertexPtr[0][0], vertexPtr[0][1], 0.0),
            QVector3D(vertexPtr[lastIndex][0], vertexPtr[lastIndex][1], 0.0));
}

void GridGeometry::fillVertexData(QByteArray &vertexData)
{
    const int numSubdivs = 1; // number of subdivision lines (i.e. lines between main grid lines)
    const int vtxSize = int(sizeof(float)) * 3 * 2;
    const int size = m_isCenterLine ? vtxSize
                  : m_isSubdivision ? 4 * m_lines * vtxSize * numSubdivs
                                    : 4 * m_lines * vtxSize;
    vertexData.resize(size);
    float *dataPtr = reinterpret_cast<float *>(vertexData.data());

    float x0 = -float(m_lines) * m_step;
    float y0 = x0;
    float x1 = -x0;
    float y1 = x1;

    if (m_isCenterLine) {
        // start position
        dataPtr[0] = 0.f;
        dataPtr[1] = y0;
        dataPtr[2] = 0.f;
        // end position
        dataPtr[3] = 0.f;
        dataPtr[4] = y1;
        dataPtr[5] = 0.f;
    } else {
        // Lines are created so that bounding box can later be calculated from first and last vertex
        if (m_isSubdivision) {
            const float subdivStep = m_step / float(numSubdivs + 1);
            const int subdivMainLines = m_lines * 2;
            auto generateSubLines = [&](float x0, float y0, float x1, float y1, bool vertical) {
                for (int i = 0; i < subdivMainLines; ++i) {
                    for (int j = 1; j <= numSubdivs; ++j) {
                        // start position
                        dataPtr[0] = vertical ? x0 + i * m_step + j * subdivStep : x0;
                        dataPtr[1] = vertical ? y0 : y0 + i * m_step + j * subdivStep;
                        dataPtr[2] = .0f;
                        // end position
                        dataPtr[3] = vertical ? x0 + i * m_step + j * subdivStep : x1;
                        dataPtr[4] = vertical ? y1 : y0 + i * m_step + j * subdivStep;
                        dataPtr[5] = .0f;
                        dataPtr += 6;
                    }
                }
            };
            generateSubLines(x0, y0, x1, y1, true);
            generateSubLines(x0, y0, x1, y1, false);
        } else {
            auto generateLines = [this, &dataPtr](float x0, float y0, float x1, float y1, bool vertical) {
                for (int i = 0; i < m_lines; ++i) {
                    // start position
                    dataPtr[0] = vertical ? x0 + i * m_step : x0;
                    dataPtr[1] = vertical ? y0 : y0 + i * m_step;
                    dataPtr[2] = .0f;
                    // end position
                    dataPtr[3] = vertical ? x0 + i * m_step : x1;
                    dataPtr[4] = vertical ? y1 : y0 + i * m_step;
                    dataPtr[5] = .0f;
                    dataPtr += 6;
                }
            };
            generateLines(x0, y0, x1, y1, true);
            generateLines(x0, y0, x1, y1, false);
            generateLines(x0, m_step, x1, y1, false);
            generateLines(m_step, y0, x1, y1, true);
        }
    }
}

}
}

#endif // QUICK3D_MODULE
