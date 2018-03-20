#pragma once

#include "AbstractStdTable.h"
#include <QMutex>

class ZehSymbolTable : public AbstractStdTable
{
    Q_OBJECT
public:
    ZehSymbolTable(QWidget* parent = nullptr);

    QString getCellContent(int r, int c) override;
    bool isValidIndex(int r, int c) override;
    void sortRows(int column, bool ascending) override;

    friend class SymbolView;
    friend class SearchListViewSymbols;

protected:
    QString paintContent(QPainter* painter, dsint rowBase, int rowOffset, int col, int x, int y, int w, int h) override;
    void updateColors() override;

private:
    std::vector<duint> mModules;
    std::vector<SYMBOLPTR> mData;
    QMutex mMutex;
    QString highlightText;

    enum
    {
        ColAddr,
        ColType,
        ColDecorated,
        ColUndecorated
    };

    QColor mCipBackgroundColor;
    QColor mCipColor;
    QColor mBreakpointBackgroundColor;
    QColor mBreakpointColor;
    QColor mHardwareBreakpointBackgroundColor;
    QColor mHardwareBreakpointColor;
    QColor mBookmarkBackgroundColor;
    QColor mBookmarkColor;
    QColor mLabelColor;
    QColor mLabelBackgroundColor;
    QColor mSelectedAddressBackgroundColor;
    QColor mSelectedAddressColor;
    QColor mAddressBackgroundColor;
    QColor mAddressColor;
    QColor mTracedBackgroundColor;
    QColor mTracedSelectedAddressBackgroundColor;
    bool bCipBase;
};
