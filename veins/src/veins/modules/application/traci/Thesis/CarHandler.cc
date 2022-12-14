#include "veins/modules/application/traci/Thesis/CarHandler.h"

using namespace veins;
using namespace omnetpp;

Define_Module(veins::CarHandler);

// ---------------------------------------------------------------------- //

void CarHandler::initialize(int stage)
{
    // Initializing variables
    DemoBaseApplLayer::initialize(stage);

    if (stage == 0)
    { 
        // TODO: Check what this variable actually does
        sentMessage = false;

        currentSubscribedServiceId = -1;
        messagesToDelete = 5;                                   // Arbitrarily choose an amount of messages to delete
        radioRange = 300;

        lastDroveAt = simTime();

        policy = cachingPolicy::FIFO;
    }
}

void CarHandler::onWSA(DemoServiceAdvertisment* wsa)
{
    // If no assigned service, assign one
    if (currentSubscribedServiceId == -1)
    {
        mac->changeServiceChannel(static_cast<Channel>(wsa->getTargetChannel()));

        currentSubscribedServiceId = wsa->getPsid();
        if (currentOfferedServiceId != wsa->getPsid()) {
            stopService();
            startService(static_cast<Channel>(wsa->getTargetChannel()), wsa->getPsid(), "Mirrored Traffic Service");
        }
    }
}

void CarHandler::onWSM(BaseFrame1609_4* frame)
{
    Message* wsm = check_and_cast<Message*>(frame);
    int distance = int(traci->getDistance(curPosition, wsm->getSenderPosition(), false));

    //Reject message if distance exceeds signal range
    if (distance < radioRange && acceptMessage(wsm))
    {
        // Change color if you accepted the message
        findHost()->getDisplayString().setTagArg("i", 1, "green");
        
        messageType type = wsm->getType();

        switch (type)
        {
            case messageType::BROADCAST:
                handleBroadcast(wsm);
                break;
            case messageType::REQUEST:
                handleRequest(wsm);
                break;
            case messageType::REPLY:
                handleReply(wsm);
                break;
            case messageType::RSU_REQUEST:
                handleRsuRequest(wsm);
                break;
            case messageType::CENTRALITY_REQUEST:
                handleCentralityRequest(wsm);
                break;
            case messageType::CENTRALITY_REPLY:
                handleCentralityReply(wsm);
                break;
            case messageType::RSU_REPLY:
                handleRsuReply(wsm);
                break;
            default:
                break;
        }
    }
}

// ---------------------------------------------------------------------- //

void CarHandler::handleSelfMsg(cMessage* msg)
{
    // This code only runs when channel switching is enabled
    if (Message* wsm = dynamic_cast<Message*>(msg))
    {
        currentState state = wsm->getState();

        switch (state)
        {
            case currentState::INITIALIZING:
                break;
            case currentState::SENDING:
                sendingMessage(wsm);
                break;
            case currentState::COLLECTING:
                collectingMessage(wsm);
                break;
            case currentState::CACHING:
                cachingMessage();
                break;
            default:
                break;
        }
    }

    else
        DemoBaseApplLayer::handleSelfMsg(msg);
}

void CarHandler::sendingMessage(Message* wsm)
{
    // If the message doesn't exceed max hop count, 
    if (wsm->getHops() < wsm->getMaxHops())
    {
        wsm->setHops(wsm->getHops() + 1);
        sendDown(wsm->dup());
    }

    else
    {
        EV << "Node with ID " << myId << ", deleting message with ID " << wsm->getSenderAddress() << endl;

        stopService();
        delete(wsm);
    }
}

void CarHandler::collectingMessage(Message* wsm)
{
    Message* reply = new Message();
    populateWSM(reply);

    reply->setSenderAddress(myId);
    reply->setSenderPosition(curPosition);
    reply->setTarget(wsm->getTarget());
    reply->setMaxHops(20);
    reply->setType(messageType::RSU_REPLY);
    reply->setCentrality(selectedCentrality::BETWEENNESS);

    std::list<Tuple>::iterator i;
    int counter = 0;

    // TODO: Fix this to handle multiple RSUs
    for (i = distanceList.begin(); i != distanceList.end(); i++)
    {
        if (!i->rsu.empty())
        {
            std::list<long>::iterator j;
            
            for (j = i->rsu.begin(); j != i->rsu.end(); j++)
            {
                if (*j == wsm->getTarget())
                {
                    reply->setCentralityData(reply->getCentralityData() + 1);
                    break;
                }
            }
        }
    }

    scheduleAt(simTime() + 0.1 + uniform(0.01, 0.2), reply);
}

// ---------------------------------------------------------------------- //

// TODO: Create a fix in the case of the car leaving the broadcast range before it can send the reply

// A way to implement this could be when replying, wait for an acknowledgement from the person you sent the reply to
// If the acknowledgement doenst come within 2s, resend the message as a broadcast 

// ---------------------------------------------------------------------- //

// Functions for handling different types of caching policies
void CarHandler::cachingMessage()
{
    switch (policy)
    {
        case cachingPolicy::FIFO:
            cachingFIFO();
            break;
        case cachingPolicy::LRU:
            cachingLRU();
            break;
        case cachingPolicy::LFU:
            cachingLFU();
            break;
        default:
            break;
    }
    
    Message* cache = new Message();
    populateWSM(cache);

    cache->setState(currentState::CACHING);
    scheduleAt(simTime() + 60 + uniform(0.01, 0.2), cache);
}

void CarHandler::cachingFIFO()
{
    if (messageList.size() < messagesToDelete)
        messageList.clear();

    else 
    {
        std::list<Tuple>::iterator end = messageList.end();
        std::list<Tuple>::iterator start = end;

        for (int i = 0; i < messagesToDelete; i++)
            start--;

        messageList.erase(start, end);
    }
}

void CarHandler::cachingLRU()
{
    if (messageList.size() < messagesToDelete)
        messageList.clear();

    else 
    {
        mergeSort(messageList);

        std::list<Tuple>::iterator end = messageList.end();
        std::list<Tuple>::iterator start = end;

        for (int i = 0; i < messagesToDelete; i++)
            start--;

        messageList.erase(start, end);
    }
}

void CarHandler::cachingLFU()
{   
    if (messageList.size() < messagesToDelete)
        messageList.clear();

    else 
    {
        mergeSort(messageList);
        
        std::list<Tuple>::iterator end = messageList.end();
        std::list<Tuple>::iterator start = end;

        for (int i = 0; i < messagesToDelete; i++)
            start--;

        messageList.erase(start, end);
    }
}

// ---------------------------------------------------------------------- //

void CarHandler::handlePositionUpdate(cObject* obj)
{
    DemoBaseApplLayer::handlePositionUpdate(obj);

    // If the car has stopped for at least 10s, it has crashed
    // TODO: Bring it up to date
    if (mobility->getSpeed() < 1)
    {
        if (simTime() - lastDroveAt >= 10 && sentMessage == false)
        {
           // Change car color to red, and start broadcasting
            findHost()->getDisplayString().setTagArg("i", 1, "red");
            sentMessage = true;

            // Create new message
            Message* wsm = new Message();

            populateWSM(wsm);

            wsm->setSenderAddress(myId);
            wsm->setSenderPosition(curPosition);
            wsm->setRoadData(mobility->getRoadId().c_str());

            // The host is standing still due to crash
            if (dataOnSch) 
            {
                startService(Channel::sch2, 42, "Traffic Information Service");

                // started service and server advertising, schedule message to self to send later
                scheduleAt(computeAsynchronousSendingTime(1, ChannelType::service), wsm);
            }
            else 
            {
                // Send right away on CCH, because channel switching is disabled
                sendDown(wsm);
            }
        }
    }

    else 
    {
        lastDroveAt = simTime();

        // Random chance of requesting infomation
        /*
        int willRequest = uniform(0, 400); 

        if (willRequest == 0)
            requestInfo();
        
        // Alternate condition, for debugging
        if (lastDroveAt == 85 && myId == 58)
                requestInfo();
        */
    }
}

// ---------------------------------------------------------------------- //

// Functions for handling different types of messages
void CarHandler::handleBroadcast(Message* wsm)
{
    //Resend the message after 2s + random delay
    wsm->setSenderPosition(curPosition);

    scheduleAt(simTime() + 2 + uniform(0.01, 0.2), wsm);
}

void CarHandler::handleRequest(Message* wsm)
{
    // If you have info, send it and continue the broadcast

    // Variables for the message list
    std::list<Tuple>::iterator i;
    float delay = 1.0;

    // Create a reply for each one of your info messages
    for (i = messageList.begin(); i != messageList.end(); i++)
    {
        if (!i->roadData.empty())
        {
            // Change last used, and used frequency
            i->usedFrequency += 1;
            i->lastUsed = simTime();

            Message* reply = new Message();

            populateWSM(reply);   

            reply->setSenderAddress(myId);
            reply->setSenderPosition(curPosition);
            reply->setTarget(wsm->getSenderAddress());
            reply->setType(messageType::REPLY);
            reply->setRoadData(i->roadData.c_str());

            delay += 0.1;
            
            messageList.push_front(Tuple(reply));
            scheduleAt(simTime() + delay + uniform(0.01, 0.2), reply);
        }
    }

    // Then forward the message to reach others who might have info
    wsm->setSenderPosition(curPosition);
    scheduleAt(simTime() + 2 + uniform(0.01, 0.2), wsm);
}

void CarHandler::handleReply(Message* wsm)
{
    // TODO: Remove from commment, when it's actual implementation time
    //Change the route if you can, to avoid the obstacle
    /*
    if (mobility->getRoadId()[0] != ':')
        traciVehicle->changeRoute(roadInfo, 9999);
    */

    // If the message was not meant for you, forward
    if (wsm->getTarget() != myId)
    {
        //Resend the message after 2s + delay
        wsm->setSenderPosition(curPosition);

        scheduleAt(simTime() + 2 + uniform(0.01, 0.2), wsm);
    }

    // If the message was meant for you, accept
    else 
    {
        findHost()->getDisplayString().setTagArg("i", 1, "gold");

        EV << "Message received after: " << wsm->getHops() + 1 << " hops.\n";
    }
}

// ---------------------------------------------------------------------- //

void CarHandler::handleRsuRequest(Message* wsm)
{
    EV << wsm->getCentrality() << endl;
    selectedCentrality centrality = wsm->getCentrality();

    switch(centrality)
    {
        case (selectedCentrality::NONE):
            break;
        case (selectedCentrality::DEGREE):
            degreeRequest(wsm);
            break;
        case (selectedCentrality::CLOSENESS):
            closenessRequest(wsm);
            break;
        case(selectedCentrality::BETWEENNESS):
            betweennessRequest(wsm);
            break;
        default:
             break;
    }
}

// Functions for handling different centrality calculations
void CarHandler::degreeRequest(Message* wsm)
{
    EV << "Algorithm for degree centrality called..." << endl;

    Message* reply = new Message();

    populateWSM(reply);

    reply->setSenderAddress(myId);
    reply->setSenderPosition(curPosition);
    reply->setRecipientAddress(wsm->getSenderAddress());
    reply->setTarget(wsm->getSenderAddress());
    reply->setType(messageType::RSU_REPLY);
    reply->setCentrality(selectedCentrality::DEGREE);

    messageList.push_front(Tuple(reply));
    scheduleAt(simTime() + 0.1 + uniform(0.01, 0.2), reply);
}

void CarHandler::closenessRequest(Message* wsm)
{
    EV << "Algorithm for closeness centrality called...\n";
    shortestPaths(wsm);
}

void CarHandler::betweennessRequest(Message* wsm)
{
    EV << "Algorithm for betweenness centrality called...\n";
    
    // Create request to calculate shortest paths to node
    Message* request = new Message();
    populateWSM(request);

    // TODO: Maybe you dont need the addresses after all...
    request->setSenderAddress(myId);
    request->setSenderPosition(curPosition);
    request->setSource(wsm->getSenderAddress());

    std::list<long> path = wsm->getPathList();
    path.push_front(myId);

    request->setPathList(path);
    request->setMaxHops(20);
    request->setType(messageType::CENTRALITY_REQUEST);
    request->setCentrality(selectedCentrality::BETWEENNESS);

    messageList.push_front(Tuple(request));
    scheduleAt(simTime() + 0.1 + uniform(0.01, 0.2), request);

    // Broadcast the request of the RSU to other cars
    wsm->setSenderPosition(curPosition);
    scheduleAt(simTime() + 0.2 + uniform(0.01, 0.2), wsm->dup());

    // After 5s, call collection to create RSU Reply
    Message* collect = new Message();
    populateWSM(collect);

    collect->setTarget(wsm->getSenderAddress());
    collect->setState(currentState::COLLECTING);

    scheduleAt(simTime() + 5 + uniform(0.01, 0.2), collect);
}

// ---------------------------------------------------------------------- //

void CarHandler::handleCentralityRequest(Message* wsm)
{
    EV << "Handling car request...\n";
    shortestPaths(wsm);
}

void CarHandler::handleCentralityReply(Message* wsm)
{
    std::list<long> path = wsm->getPathList();

    // If you're the one who made the request, then gather the data and send to the RSU
    if (path.empty())
    {
        EV << "Answer to my request received, node " << myId << endl;
        std::list<Tuple>::iterator i;
        bool insert = true;

        for (i = distanceList.begin(); i != distanceList.end(); i++)
        {
            if (i->id == wsm->getSenderAddress())
            {
                insert = false;

                if (i->hops > wsm->getHops())
                {
                    i->hops = wsm->getHops();
                    i->rsu = wsm->getRsuList();
                }
                else if (i->hops == wsm->getHops())
                {
                    std::list<long> messageRsu = wsm->getRsuList();
                    std::list<long> savedRsu = i->rsu;

                    if (messageRsu.size() > savedRsu.size())
                        i->rsu = wsm->getRsuList();
                }

                break;
            }
        }

        if (insert)
            distanceList.push_front(Tuple(wsm));


        for (i = distanceList.begin(); i != distanceList.end(); i++)
            EV << "Node with ID " << i->id << " and distance " << i->hops << endl;

    }

    else
    {
        wsm->setSenderPosition(curPosition);
        wsm->setRecipientAddress(*path.begin());

        path.pop_front();
        
        wsm->setPathList(path);

        scheduleAt(simTime() + 0.1 + uniform(0.01, 0.2), wsm->dup());
    }
}

// ---------------------------------------------------------------------- //

void CarHandler::handleRsuReply(Message* wsm)
{
    selectedCentrality centrality = wsm->getCentrality();

    switch (centrality)
    {
        case (selectedCentrality::NONE):
            break;
        case (selectedCentrality::DEGREE):
            break;
        case (selectedCentrality::CLOSENESS):
            closenessReply(wsm);
            break;
        case (selectedCentrality::BETWEENNESS):
            betweennessReply(wsm);
            break;
    }
}

// Functions for handling replies to centrality requests
void CarHandler::closenessReply(Message* wsm)
{        
    std::list<long> path =  wsm->getPathList();
    std::list<long>::iterator i;

    for (i = path.begin(); i != path.end(); i++)
        EV << "Node in path with id: " << *i << endl;

    wsm->setSenderPosition(curPosition);
    wsm->setRecipientAddress(*path.begin());    
    
    path.pop_front();

    wsm->setPathList(path);  

    // TODO: Check the dup method
    scheduleAt(simTime() + 0.1 + uniform(0.01, 0.2), wsm->dup());
}

void CarHandler::betweennessReply(Message* wsm)
{
    EV << "Forwarding RSU reply...\n";

    EV << "Count is " << wsm->getCentralityData() << endl;
    // Forward the message until it reaches the RSU
    wsm->setSenderPosition(curPosition);

    scheduleAt(simTime() + 0.1 + uniform(0.01, 0.2), wsm->dup());
}

// ---------------------------------------------------------------------- //

// Function for requesting info about network status
void CarHandler::requestInfo()
{
    // Change color of requesting car to purple
    findHost()->getDisplayString().setTagArg("i", 1, "purple");
    sentMessage = true;

    Message* request = new Message();
    populateWSM(request);

    request->setSenderAddress(myId);
    request->setSenderPosition(curPosition);
    request->setType(messageType::REQUEST);
    
    messageList.push_front(Tuple(request));

    scheduleAt(simTime() + 1, request);
}

void CarHandler::shortestPaths(Message* wsm)
{    
    std::list<long> path = wsm->getPathList();

    // If your id isn't already in the path list
    if (!inPath(path))
    {
        EV << "Creating shortest path reply, node " << myId << endl;

        Message* reply = new Message();
        populateWSM(reply);

        reply->setSenderAddress(myId);
        reply->setSenderPosition(curPosition);
        reply->setRecipientAddress(*path.begin());
        reply->setTarget(wsm->getSource());
        reply->setSource(wsm->getSenderAddress());
        reply->setMaxHops(20);                          // Changing the no. of hops, because it has to go through the entire graph
        
        path.pop_front();

        reply->setPathList(path);

        if (wsm->getType() == messageType::RSU_REQUEST)
        {
            reply->setType(messageType::RSU_REPLY);
            reply->setCentrality(selectedCentrality::CLOSENESS);
        }
        else 
        {
            reply->setType(messageType::CENTRALITY_REPLY);
            reply->setCentrality(selectedCentrality::BETWEENNESS);
            reply->setRsuList(wsm->getRsuList());
        }
        
        messageList.push_front(Tuple(reply));
        scheduleAt(simTime() + 0.1 + uniform(0.01, 0.2), reply);
        EV << "Reply sent, message ID: " << reply->getSenderAddress() << endl;

        // Add your id to the path list and broadcast it again
        path = wsm->getPathList();
        path.push_front(myId);

        wsm->setPathList(path);
        wsm->setSenderPosition(curPosition);

        scheduleAt(simTime() + 0.2 + uniform(0.01, 0.2), wsm->dup());
        EV << "Message forwarded, message ID: " << wsm->getSenderAddress() << endl;
    }
    
}

void CarHandler::mergeSort(std::list<Tuple>& array)
{
    int midpoint = array.size() / 2;
    if (array.size() == 1)
        return;

    std::list<Tuple>::iterator i = array.begin();

    std::list<Tuple> left;
    while (left.size() < midpoint)
    {
        left.push_back(*i);
        i++;
    }
        
    std::list<Tuple> right;
    while (right.size() < array.size() - midpoint)
    {
        right.push_back(*i);
        i++;
    }
    
    mergeSort(left);
    mergeSort(right);

    if (policy == cachingPolicy::LRU)
        mergeLRU(array, left, right);
    else if (policy == cachingPolicy::LFU)
        mergeLFU(array, left, right);
}

void CarHandler::mergeLRU(std::list<Tuple> &array, std::list<Tuple> left, std::list<Tuple> right)
{
    std::list<Tuple>::iterator i = left.begin();
    std::list<Tuple>::iterator j = right.begin();
    std::list<Tuple>::iterator k = array.begin();
    
    while (i != left.end() && j != right.end())
    {
        if (i->lastUsed >= j->lastUsed)
        {
            *k = *i;
            i++;
            k++;
        }
        
        else if (j->lastUsed > i->lastUsed)
        {            
            *k = *j;
            j++;
            k++;
        }
    }
    
    while (i != left.end())
    {
        *k = *i;
        i++;
        k++;
    }
    
    while (j != right.end())
    {
        *k = *j;
        j++;
        k++;
    }
}

void CarHandler::mergeLFU(std::list<Tuple> &array, std::list<Tuple> left, std::list<Tuple> right)
{
    std::list<Tuple>::iterator i = left.begin();
    std::list<Tuple>::iterator j = right.begin();
    std::list<Tuple>::iterator k = array.begin();
    
    while (i != left.end() && j != right.end())
    {
        if (i->usedFrequency >= j->usedFrequency)
        {
            *k = *i;
            i++;
            k++;
        }
        
        else if (j->usedFrequency > i->usedFrequency)
        {            
            *k = *j;
            j++;
            k++;
        }
    }
    
    while (i != left.end())
    {
        *k = *i;
        i++;
        k++;
    }
    
    while (j != right.end())
    {
        *k = *j;
        j++;
        k++;
    }
}

bool CarHandler::acceptMessage(Message* wsm)
{
    EV << "Node with ID " << myId << " received message from " << wsm->getSenderAddress() << endl;

    if (!messageList.empty())
    {
        std::list<Tuple>::iterator i;
        for (i = messageList.begin(); i != messageList.end(); i++)
        {
            // Depending on the timestamp of the message, put it in the correct position
            if (i->timestamp > wsm->getCreationTime())
                continue;

            if (i->timestamp == wsm->getCreationTime())
                if (i->id == wsm->getSenderAddress())
                    return false;

            if (i->timestamp < wsm->getCreationTime())
            {
                messageList.insert(i--, Tuple(wsm));
                return true;
            }
        }
    }

    messageList.push_front(Tuple(wsm));
    return true;
}

bool CarHandler::inPath(std::list<long> path)
{
    EV << "I'm here, node with ID " << myId << endl;

    std::list<long>::iterator i;

    for (i = path.begin(); i != path.end(); i++)
    {
        if (*i == myId)
            return true;
    }

    return false;
}

// ---------------------------------------------------------------------- //
