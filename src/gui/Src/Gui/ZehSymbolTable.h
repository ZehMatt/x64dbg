#pragma once

#include "AbstractStdTable.h"
#include <QMutex>

class ZehSymbolTable : public AbstractStdTable
{
    Q_OBJECT
public:
    ZehSymbolTable(QWidget* parent);

    QString getCellContent(int r, int c) override;
    bool isValidIndex(int r, int c) override;
    void sortRows(int column, bool ascending) override;

    friend class SymbolView;

private:
    std::vector<duint> mModules;
    std::vector<SYMBOLPTR> mData;
    QMutex mMutex;

    enum
    {
        ColAddr,
        ColType,
        ColDecorated,
        ColUndecorated
    };
};
