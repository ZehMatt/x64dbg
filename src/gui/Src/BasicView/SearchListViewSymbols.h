#ifndef SEARCHLISTVIEWSYMBOLS_H
#define SEARCHLISTVIEWSYMBOLS_H

#include <QWidget>
#include <QLineEdit>
#include <QCheckBox>
#include "ZehSymbolTable.h"
#include "MenuBuilder.h"
#include "ActionHelpers.h"

class SearchListViewSymbols : public QWidget, public ActionHelper<SearchListViewSymbols>
{
    Q_OBJECT

public:
    explicit SearchListViewSymbols(bool EnableRegex = true, QWidget* parent = 0, bool EnableLock = false);
    ~SearchListViewSymbols();

    ZehSymbolTable* mList;
    ZehSymbolTable* mSearchList;
    ZehSymbolTable* mCurList;
    QLineEdit* mSearchBox;
    int mSearchStartCol;

    bool findTextInList(ZehSymbolTable* list, QString text, int row, int startcol, bool startswith);
    void refreshSearchList();

    bool isSearchBoxLocked();

private slots:
    void searchTextChanged(const QString & arg1);
    void listContextMenu(const QPoint & pos);
    void doubleClickedSlot();
    void searchSlot();
    void on_checkBoxRegex_stateChanged(int state);
    void on_checkBoxLock_toggled(bool checked);

signals:
    void enterPressedSignal();
    void listContextMenuSignal(QMenu* wMenu);
    void emptySearchResult();

protected:
    bool eventFilter(QObject* obj, QEvent* event);

private:
    QCheckBox* mRegexCheckbox;
    QCheckBox* mLockCheckbox;
    QAction* mSearchAction;
};

#endif // SEARCHLISTVIEWSYMBOLS_H
