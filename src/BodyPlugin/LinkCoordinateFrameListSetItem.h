#ifndef CNOID_BODY_PLUGIN_LINK_COORDINATE_FRAME_LIST_SET_ITEM_H
#define CNOID_BODY_PLUGIN_LINK_COORDINATE_FRAME_LIST_SET_ITEM_H

#include <cnoid/MultiCoordinateFrameListItem>
#include <cnoid/LinkCoordinateFrameSet>
#include <cnoid/Signal>
#include "exportdecl.h"

namespace cnoid {

class CNOID_EXPORT LinkCoordinateFrameListSetItem : public MultiCoordinateFrameListItem
{
public:
    static void initializeClass(ExtensionManager* ext);

    static SignalProxy<void(LinkCoordinateFrameListSetItem* frameListSetItem)> sigInstanceAddedOrUpdated();

    enum FrameType {
        WorldFrame = LinkCoordinateFrameSet::WorldFrame,
        BodyFrame  = LinkCoordinateFrameSet::BodyFrame,
        LinkFrame   = LinkCoordinateFrameSet::LinkFrame
    };

    static void setFrameListLabels(
        const char* worldFrameLabel, const char* bodyFrameLabel, const char* linkFrameLabel);

    static void setFrameListEnabledForAllItems(FrameType type, bool on);

    LinkCoordinateFrameListSetItem();
    LinkCoordinateFrameListSetItem(const LinkCoordinateFrameListSetItem& org);

    LinkCoordinateFrameSet* frameSets();
    const LinkCoordinateFrameSet* frameSets() const;

    CoordinateFrameListItem* worldFrameListItem(int index);
    const CoordinateFrameListItem* worldFrameListItem(int index) const;

    CoordinateFrameListItem* bodyFrameListItem(int index);
    const CoordinateFrameListItem* bodyFrameListItem(int index) const;

    CoordinateFrameListItem* linkFrameListItem(int index);
    const CoordinateFrameListItem* linkFrameListItem(int index) const;

protected:
    virtual Item* doDuplicate() const override;
    virtual void onPositionChanged() override;

private:
    void initializeFrameListEnabling();
    
    ScopedConnection connection;
};

typedef ref_ptr<LinkCoordinateFrameListSetItem> LinkCoordinateFrameListSetItemItemPtr;

}

#endif
