#include "PositionTagListWidget.h"
#include "MenuManager.h"
#include "PositionTagGroupItem.h"
#include <cnoid/PositionTagGroup>
#include <cnoid/PositionTag>
#include <cnoid/EigenUtil>
#include <cnoid/ConnectionSet>
#include <QHeaderView>
#include <QAbstractTableModel>
#include <QStyledItemDelegate>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QGuiApplication>
#include <fmt/format.h>
#include "gettext.h"

using namespace std;
using namespace cnoid;
using fmt::format;

namespace {

constexpr int NumColumns = 2;
constexpr int IndexColumn = 0;
constexpr int PositionColumn = 1;

class TagGroupModel : public QAbstractTableModel
{
public:
    PositionTagListWidget* widget;
    PositionTagGroupItemPtr tagGroupItem;
    ScopedConnectionSet tagGroupConnections;
    QFont monoFont;
    
    TagGroupModel(PositionTagListWidget* widget);
    void setTagGroupItem(PositionTagGroupItem* tagGroupItem);
    int numTags() const;
    PositionTag* tagAt(const QModelIndex& index) const;
    virtual int rowCount(const QModelIndex& parent) const override;
    virtual int columnCount(const QModelIndex& parent) const override;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    virtual QVariant data(const QModelIndex& index, int role) const override;
    QVariant getPositionData(const PositionTag* tag) const;
    void onTagAdded(int tagIndex);
    void onTagRemoved(int tagIndex);
    void onTagUpdated(int tagIndex);
};

}

namespace cnoid {

class PositionTagListWidget::Impl
{
public:
    PositionTagGroupItemPtr tagGroupItem;
    TagGroupModel* tagGroupModel;
    bool isSelectionChangedAlreadyCalled;
    MenuManager contextMenuManager;
    Signal<void(const std::vector<int>& selected)> sigTagSelectionChanged;
    vector<int> selectedTagIndices;
    Signal<void(int tagIndex)> sigTagPressed;
    Signal<void(int tagIndex)> sigTagDoubleClicked;
    Signal<void(MenuManager& menu)> sigContextMenuRequest;
};

}


TagGroupModel::TagGroupModel(PositionTagListWidget* widget)
    : QAbstractTableModel(widget),
      widget(widget),
      monoFont("Monospace")
{
    monoFont.setStyleHint(QFont::TypeWriter);
}


void TagGroupModel::setTagGroupItem(PositionTagGroupItem* tagGroupItem)
{
    beginResetModel();

    this->tagGroupItem = tagGroupItem;

    tagGroupConnections.disconnect();
    if(tagGroupItem){
        auto tags = tagGroupItem->tagGroup();
        tagGroupConnections.add(
            tags->sigTagAdded().connect(
                [&](int index){ onTagAdded(index); }));
        tagGroupConnections.add(
            tags->sigTagRemoved().connect(
                [&](int index, PositionTag*){ onTagRemoved(index); }));
        tagGroupConnections.add(
            tags->sigTagUpdated().connect(
                [&](int index){ onTagUpdated(index); }));
    }
            
    endResetModel();
}


int TagGroupModel::numTags() const
{
    if(tagGroupItem){
        return tagGroupItem->tagGroup()->numTags();
    }
    return 0;
}


PositionTag* TagGroupModel::tagAt(const QModelIndex& index) const
{
    if(!index.isValid()){
        return nullptr;
    }
    return tagGroupItem->tagGroup()->tagAt(index.row());
}
        
    
int TagGroupModel::rowCount(const QModelIndex& parent) const
{
    int n = 0;
    if(!parent.isValid()){
        n = numTags();
    }
    if(n == 0){ // to show an empty row
        n = 1;
    }
    return n;
}


int TagGroupModel::columnCount(const QModelIndex& parent) const
{
    return NumColumns;
}
        

QVariant TagGroupModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role == Qt::DisplayRole){
        if(orientation == Qt::Horizontal){
            switch(section){
            case IndexColumn:
                return " No ";
            case PositionColumn:
                return _("Position");
            default:
                return QVariant();
            }
        } else {
            return QString::number(section);
        }
    } else if(role == Qt::TextAlignmentRole){
        if(orientation == Qt::Horizontal){
            return Qt::AlignCenter;
        }
    }
    return QVariant();
}


QModelIndex TagGroupModel::index(int row, int column, const QModelIndex& parent) const
{
    if(!tagGroupItem || parent.isValid()){
        return QModelIndex();
    }
    if(row < numTags()){
        return createIndex(row, column);
    }
    return QModelIndex();
}
    

QVariant TagGroupModel::data(const QModelIndex& index, int role) const
{
    auto tag = tagAt(index);
    if(!tag){
        return QVariant();
    }
    int column = index.column();
    if(role == Qt::DisplayRole || role == Qt::EditRole){
        switch(column){
        case IndexColumn:
            return index.row();

        case PositionColumn:
            return getPositionData(tag);

        default:
            break;
        }
    } else if(role == Qt::TextAlignmentRole){
        if(column == PositionColumn){
            return (Qt::AlignLeft + Qt::AlignVCenter);
        } else {
            return Qt::AlignCenter;
        }
    } else if(role == Qt::FontRole){
        if(column == PositionColumn){
            return monoFont;
        }
    }
    return QVariant();
}


QVariant TagGroupModel::getPositionData(const PositionTag* tag) const
{
    auto p = tag->translation();
    if(!tag->hasAttitude()){
        return format("{0: 1.3f} {1: 1.3f} {2: 1.3f}", p.x(), p.y(), p.z()).c_str();
    } else {
        auto rpy = degree(rpyFromRot(tag->rotation()));
        return format("{0: 1.3f} {1: 1.3f} {2: 1.3f} {3: 6.1f} {4: 6.1f} {5: 6.1f}",
                      p.x(), p.y(), p.z(), rpy[0], rpy[1], rpy[2]).c_str();
    }
}



void TagGroupModel::onTagAdded(int tagIndex)
{
    if(numTags() == 0){
        // Remove the empty row first
        beginRemoveRows(QModelIndex(), 0, 0);
        endRemoveRows();
    }
    beginInsertRows(QModelIndex(), tagIndex, tagIndex);
    endInsertRows();

    //resizeColumnToContents(IdColumn);
    //resizeColumnToContents(PositionColumn);
}


void TagGroupModel::onTagRemoved(int tagIndex)
{
    beginRemoveRows(QModelIndex(), tagIndex, tagIndex);
    endRemoveRows();
    if(numTags() == 0){
        // This is necessary to show the empty row
        beginResetModel();
        endResetModel();
    }
}


void TagGroupModel::onTagUpdated(int tagIndex)
{
    auto modelIndex = index(tagIndex, PositionColumn, QModelIndex());
    Q_EMIT dataChanged(modelIndex, modelIndex, { Qt::EditRole });
}


PositionTagListWidget::PositionTagListWidget(QWidget* parent)
    : QTableView(parent)
{
    impl = new Impl;
    
    //setFrameShape(QFrame::NoFrame);
    
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setTabKeyNavigation(true);
    setCornerButtonEnabled(true);
    setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

    /*
    setEditTriggers(
        QAbstractItemView::DoubleClicked |
        QAbstractItemView::EditKeyPressed |
        QAbstractItemView::AnyKeyPressed);
    */

    impl->tagGroupModel = new TagGroupModel(this);
    setModel(impl->tagGroupModel);

    auto hheader = horizontalHeader();
    hheader->setMinimumSectionSize(24);
    hheader->setSectionResizeMode(IndexColumn, QHeaderView::ResizeToContents);
    hheader->setSectionResizeMode(PositionColumn, QHeaderView::Stretch);
    auto vheader = verticalHeader();
    vheader->setSectionResizeMode(QHeaderView::ResizeToContents);
    vheader->hide();

    connect(this, &QTableView::pressed,
            [this](const QModelIndex& index){
                if(index.isValid() && QGuiApplication::mouseButtons() == Qt::LeftButton){
                    if(!impl->isSelectionChangedAlreadyCalled){
                        impl->sigTagPressed(index.row());
                    }
                }
            });

    connect(this, &QTableView::doubleClicked,
            [this](const QModelIndex& index){
                if(index.isValid()){
                    impl->sigTagDoubleClicked(index.row());
                }
            });
}


void PositionTagListWidget::setTagGroupItem(PositionTagGroupItem* item)
{
    impl->tagGroupItem = item;
    impl->tagGroupModel->setTagGroupItem(item);
}


int PositionTagListWidget::currentTagIndex() const
{
    auto current = selectionModel()->currentIndex();
    return current.isValid() ? current.row() : impl->tagGroupModel->numTags();
}


void PositionTagListWidget::setCurrentTagIndex(int tagIndex)
{
    selectionModel()->setCurrentIndex(
        impl->tagGroupModel->index(tagIndex, 0, QModelIndex()),
        QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows | QItemSelectionModel::Clear);
}


std::vector<int> PositionTagListWidget::selectedTagIndices() const
{
    std::vector<int> indices;
    for(auto& index : selectionModel()->selectedRows()){
        indices.push_back(index.row());
    }
    return indices;
}


void PositionTagListWidget::removeSelectedTags()
{
    if(impl->tagGroupItem){
        auto tags = impl->tagGroupItem->tagGroup();
        auto selected = selectionModel()->selectedRows();
        std::sort(selected.begin(), selected.end());
        int numRemoved = 0;
        for(auto& index : selected){
            int tagIndex = index.row() - numRemoved;
            tags->removeAt(tagIndex);
            ++numRemoved;
        }
    }
}


void PositionTagListWidget::keyPressEvent(QKeyEvent* event)
{
    bool processed = true;

    switch(event->key()){
    case Qt::Key_Escape:
        clearSelection();
        break;
    case Qt::Key_Delete:
        removeSelectedTags();
        break;
    default:
        processed = false;
        break;
    }
        
    if(!processed && (event->modifiers() & Qt::ControlModifier)){
        processed = true;
        switch(event->key()){
        case Qt::Key_A:
            selectAll();
            break;
        default:
            processed = false;
            break;
        }
    }

    if(!processed){
        QTableView::keyPressEvent(event);
    }
}

       
void PositionTagListWidget::mousePressEvent(QMouseEvent* event)
{
    impl->isSelectionChangedAlreadyCalled = false;
    
    QTableView::mousePressEvent(event);

    if(event->button() == Qt::RightButton){
        int row = rowAt(event->pos().y());
        if(row >= 0){
            // Show the context menu
            impl->contextMenuManager.setNewPopupMenu(this);
            impl->sigContextMenuRequest(impl->contextMenuManager);
            impl->contextMenuManager.popupMenu()->popup(event->globalPos());
        }
    }
}


void PositionTagListWidget::selectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
    impl->isSelectionChangedAlreadyCalled = true;
    
    QTableView::selectionChanged(selected, deselected);

    impl->selectedTagIndices.clear();
    for(auto& index : selected.indexes()){
        impl->selectedTagIndices.push_back(index.row());
    }

    impl->sigTagSelectionChanged(impl->selectedTagIndices);

    if(impl->tagGroupItem){
        impl->tagGroupItem->setSelectedTagIndices(impl->selectedTagIndices);
    }
}


SignalProxy<void(const std::vector<int>& selected)> PositionTagListWidget::sigTagSelectionChanged()
{
    return impl->sigTagSelectionChanged;
}


SignalProxy<void(int tagIndex)> PositionTagListWidget::sigTagPressed()
{
    return impl->sigTagPressed;
}


SignalProxy<void(int tagIndex)> PositionTagListWidget::sigTagDoubleClicked()
{
    return impl->sigTagDoubleClicked;
}



SignalProxy<void(MenuManager& menu)> PositionTagListWidget::sigContextMenuRequest()
{
    return impl->sigContextMenuRequest;
}

