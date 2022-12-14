//
// Generated file, do not edit! Created by nedtool 5.6 from veins/modules/application/traci/Thesis/Message.msg.
//

#ifndef __VEINS_MESSAGE_M_H
#define __VEINS_MESSAGE_M_H

#if defined(__clang__)
#  pragma clang diagnostic ignored "-Wreserved-id-macro"
#endif
#include <omnetpp.h>

// nedtool version check
#define MSGC_VERSION 0x0506
#if (MSGC_VERSION!=OMNETPP_VERSION)
#    error Version mismatch! Probably this file was generated by an earlier version of nedtool: 'make clean' should help.
#endif

// dll export symbol
#ifndef VEINS_API
#  if defined(VEINS_EXPORT)
#    define VEINS_API  OPP_DLLEXPORT
#  elif defined(VEINS_IMPORT)
#    define VEINS_API  OPP_DLLIMPORT
#  else
#    define VEINS_API
#  endif
#endif


namespace veins {

class Message;
} // namespace veins

#include "veins/base/utils/Coord_m.h" // import veins.base.utils.Coord

#include "veins/modules/messages/BaseFrame1609_4_m.h" // import veins.modules.messages.BaseFrame1609_4

#include "veins/base/utils/SimpleAddress_m.h" // import veins.base.utils.SimpleAddress

// cplusplus {{
    #include <list>

    typedef std::list<long> List;
// }}


namespace veins {

/**
 * Enum generated from <tt>veins/modules/application/traci/Thesis/Message.msg:17</tt> by nedtool.
 * <pre>
 * enum messageType
 * {
 *     BROADCAST = 0;
 *     REQUEST = 1;
 *     REPLY = 2;
 *     RSU_REQUEST = 3;
 *     CENTRALITY_REQUEST = 4;
 *     CENTRALITY_REPLY = 5;
 *     RSU_REPLY = 6;
 *     ACKNOWLEDGEMENT = 7;
 * }
 * </pre>
 */
enum messageType {
    BROADCAST = 0,
    REQUEST = 1,
    REPLY = 2,
    RSU_REQUEST = 3,
    CENTRALITY_REQUEST = 4,
    CENTRALITY_REPLY = 5,
    RSU_REPLY = 6,
    ACKNOWLEDGEMENT = 7
};

/**
 * Enum generated from <tt>veins/modules/application/traci/Thesis/Message.msg:29</tt> by nedtool.
 * <pre>
 * enum currentState
 * {
 *     INITIALIZING = 0;
 *     SENDING = 1;
 *     COLLECTING = 2;
 *     CACHING = 3;
 *     TESTING = 4;
 *     REPEATING = 5;
 * }
 * </pre>
 */
enum currentState {
    INITIALIZING = 0,
    SENDING = 1,
    COLLECTING = 2,
    CACHING = 3,
    TESTING = 4,
    REPEATING = 5
};

/**
 * Enum generated from <tt>veins/modules/application/traci/Thesis/Message.msg:40</tt> by nedtool.
 * <pre>
 * enum cachingPolicy
 * {
 *     FIFO = 0;
 *     LRU = 1;
 *     LFU = 2;
 * }
 * </pre>
 */
enum cachingPolicy {
    FIFO = 0,
    LRU = 1,
    LFU = 2
};

/**
 * Enum generated from <tt>veins/modules/application/traci/Thesis/Message.msg:47</tt> by nedtool.
 * <pre>
 * enum scenario
 * {
 *     CENTRALITY = 0;
 *     CACHE = 1;
 * }
 * </pre>
 */
enum scenario {
    CENTRALITY = 0,
    CACHE = 1
};

/**
 * Enum generated from <tt>veins/modules/application/traci/Thesis/Message.msg:53</tt> by nedtool.
 * <pre>
 * enum selectedCentrality
 * {
 *     DEGREE = 0;
 *     CLOSENESS = 1;
 *     BETWEENNESS = 2;
 *     NONE = 3;
 * }
 * </pre>
 */
enum selectedCentrality {
    DEGREE = 0,
    CLOSENESS = 1,
    BETWEENNESS = 2,
    NONE = 3
};

/**
 * Class generated from <tt>veins/modules/application/traci/Thesis/Message.msg:61</tt> by nedtool.
 * <pre>
 * packet Message extends BaseFrame1609_4
 * {
 *     LAddress::L2Type senderAddress = -1;
 * 
 *     LAddress::L2Type source;
 *     LAddress::L2Type target;
 * 
 *     // Message properties
 *     int testFlag = 0;
 *     int maxHops = 1;
 *     int hops = 0;
 * 
 *     // Message specifications
 *     messageType type = messageType::BROADCAST;
 *     currentState state = currentState::SENDING;
 *     selectedCentrality centrality = selectedCentrality::NONE;
 * 
 *     // Message data
 *     string roadData = "";
 *     int centralityData;
 *     simtime_t ackData;
 * 
 *     List rsuList;                                           // Variable to determine if message has passed from an RSU, might be pointless
 *     List pathList;                                          // List used for centrality calculations
 *     Coord senderPosition;                                   // Variable used for distance calculations
 * }
 * </pre>
 */
class VEINS_API Message : public ::veins::BaseFrame1609_4
{
  protected:
    LAddress::L2Type senderAddress = -1;
    LAddress::L2Type source;
    LAddress::L2Type target;
    int testFlag = 0;
    int maxHops = 1;
    int hops = 0;
    veins::messageType type = messageType::BROADCAST;
    veins::currentState state = currentState::SENDING;
    veins::selectedCentrality centrality = selectedCentrality::NONE;
    omnetpp::opp_string roadData = "";
    int centralityData = 0;
    omnetpp::simtime_t ackData = SIMTIME_ZERO;
    List rsuList;
    List pathList;
    Coord senderPosition;

  private:
    void copy(const Message& other);

  protected:
    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const Message&);

  public:
    Message(const char *name=nullptr, short kind=0);
    Message(const Message& other);
    virtual ~Message();
    Message& operator=(const Message& other);
    virtual Message *dup() const override {return new Message(*this);}
    virtual void parsimPack(omnetpp::cCommBuffer *b) const override;
    virtual void parsimUnpack(omnetpp::cCommBuffer *b) override;

    // field getter/setter methods
    virtual const LAddress::L2Type& getSenderAddress() const;
    virtual LAddress::L2Type& getSenderAddressForUpdate() { return const_cast<LAddress::L2Type&>(const_cast<Message*>(this)->getSenderAddress());}
    virtual void setSenderAddress(const LAddress::L2Type& senderAddress);
    virtual const LAddress::L2Type& getSource() const;
    virtual LAddress::L2Type& getSourceForUpdate() { return const_cast<LAddress::L2Type&>(const_cast<Message*>(this)->getSource());}
    virtual void setSource(const LAddress::L2Type& source);
    virtual const LAddress::L2Type& getTarget() const;
    virtual LAddress::L2Type& getTargetForUpdate() { return const_cast<LAddress::L2Type&>(const_cast<Message*>(this)->getTarget());}
    virtual void setTarget(const LAddress::L2Type& target);
    virtual int getTestFlag() const;
    virtual void setTestFlag(int testFlag);
    virtual int getMaxHops() const;
    virtual void setMaxHops(int maxHops);
    virtual int getHops() const;
    virtual void setHops(int hops);
    virtual veins::messageType getType() const;
    virtual void setType(veins::messageType type);
    virtual veins::currentState getState() const;
    virtual void setState(veins::currentState state);
    virtual veins::selectedCentrality getCentrality() const;
    virtual void setCentrality(veins::selectedCentrality centrality);
    virtual const char * getRoadData() const;
    virtual void setRoadData(const char * roadData);
    virtual int getCentralityData() const;
    virtual void setCentralityData(int centralityData);
    virtual omnetpp::simtime_t getAckData() const;
    virtual void setAckData(omnetpp::simtime_t ackData);
    virtual const List& getRsuList() const;
    virtual List& getRsuListForUpdate() { return const_cast<List&>(const_cast<Message*>(this)->getRsuList());}
    virtual void setRsuList(const List& rsuList);
    virtual const List& getPathList() const;
    virtual List& getPathListForUpdate() { return const_cast<List&>(const_cast<Message*>(this)->getPathList());}
    virtual void setPathList(const List& pathList);
    virtual const Coord& getSenderPosition() const;
    virtual Coord& getSenderPositionForUpdate() { return const_cast<Coord&>(const_cast<Message*>(this)->getSenderPosition());}
    virtual void setSenderPosition(const Coord& senderPosition);
};

inline void doParsimPacking(omnetpp::cCommBuffer *b, const Message& obj) {obj.parsimPack(b);}
inline void doParsimUnpacking(omnetpp::cCommBuffer *b, Message& obj) {obj.parsimUnpack(b);}

} // namespace veins

#endif // ifndef __VEINS_MESSAGE_M_H

