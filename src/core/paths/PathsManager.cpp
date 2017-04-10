#include "PathsManager.h"

PathsManager::PathsManager(
    const NodeUUID &nodeUUID,
    TrustLinesManager *trustLinesManager,
    StorageHandler *storageHandler,
    Logger *logger):

    mNodeUUID(nodeUUID),
    mTrustLinesManager(trustLinesManager),
    mStorageHandler(storageHandler),
    mLog(logger),
    mPathCollection(nullptr){

    // TODO remove from here
    //testStorageHandler();
    //fillRoutingTables();
    //fillBigRoutingTables();
    //testTrustLineHandler();
    //testPaymentStateOperationsHandler();
    //testTime();
}

void PathsManager::findDirectPath() {

    for (auto const &nodeUUID : mTrustLinesManager->rt1()) {
        if (nodeUUID == mContractorUUID) {
            Path path = Path(
                mNodeUUID,
                mContractorUUID);
            mPathCollection->add(path);
            //info() << "found direct path";
            return;
        }
    }
}

void PathsManager::findPathsOnSecondLevel() {

    DateTime startTime = utc_now();
    for (auto const &nodeUUID : mStorageHandler->routingTablesHandler()->subRoutesSecondLevel(mContractorUUID)) {
        vector<NodeUUID> intermediateNodes;
        intermediateNodes.push_back(nodeUUID);
        Path path(
            mNodeUUID,
            mContractorUUID,
            intermediateNodes);
        mPathCollection->add(path);
        //info() << "found path on second level";
    }
    /*Duration methodTime = utc_now() - startTime;
    info() << "findPathsOnSecondLevel method time: " << methodTime;*/
}

void PathsManager::findPathsOnThirdLevel() {

    DateTime startTime = utc_now();
    for (auto const &nodeUUIDAndNodeUUID : mStorageHandler->routingTablesHandler()->subRoutesThirdLevelContractor(
            mContractorUUID,
            mNodeUUID)) {
        vector<NodeUUID> intermediateNodes;
        intermediateNodes.push_back(nodeUUIDAndNodeUUID.first);
        intermediateNodes.push_back(nodeUUIDAndNodeUUID.second);
        Path path(
            mNodeUUID,
            mContractorUUID,
            intermediateNodes);
        mPathCollection->add(path);
        //info() << "found path on third level";
    }
    /*Duration methodTime = utc_now() - startTime;
    info() << "findPathsOnThirdLevel method time: " << methodTime;*/
}

void PathsManager::findPathsOnForthLevel(
    vector<NodeUUID> &contractorRT1) {

    DateTime startTime =  utc_now();
    for (auto const &nodeUUID : contractorRT1) {
        if (nodeUUID == mNodeUUID) {
            continue;
        }
        for (auto const &nodeUUIDAndNodeUUID : mStorageHandler->routingTablesHandler()->subRoutesThirdLevelWithForbiddenNodes(
                nodeUUID,
                mNodeUUID,
                mContractorUUID)) {
            vector<NodeUUID> intermediateNodes;
            intermediateNodes.push_back(nodeUUIDAndNodeUUID.first);
            intermediateNodes.push_back(nodeUUIDAndNodeUUID.second);
            intermediateNodes.push_back(nodeUUID);
            Path path(
                mNodeUUID,
                mContractorUUID,
                intermediateNodes);
            mPathCollection->add(path);
            //info() << "found path on forth level";
        }
    }
    /*Duration methodTime = utc_now() - startTime;
    info() << "findPathsOnForthLevel method time: " << methodTime;*/
}

void PathsManager::findPathsOnFifthLevel(
    unordered_map<NodeUUID, vector<NodeUUID>, boost::hash<boost::uuids::uuid>> &contractorRT2) {

    DateTime startTime = utc_now();
    for (auto const &itRT2 : contractorRT2) {
        // TODO (mc) : need or not second condition (itRT2.first == contractorUUID)
        if (itRT2.first == mNodeUUID || itRT2.first == mContractorUUID) {
            continue;
        }
        for (auto const &nodeUUIDAndNodeUUID : mStorageHandler->routingTablesHandler()->subRoutesThirdLevelWithForbiddenNodes(
                itRT2.first,
                mNodeUUID,
                mContractorUUID)) {
            vector<NodeUUID> intermediateNodes;
            intermediateNodes.push_back(nodeUUIDAndNodeUUID.first);
            intermediateNodes.push_back(nodeUUIDAndNodeUUID.second);
            intermediateNodes.push_back(itRT2.first);
            for (auto const &nodeUUID : itRT2.second) {
                if(std::find(intermediateNodes.begin(), intermediateNodes.end(), nodeUUID) != intermediateNodes.end() ||
                        nodeUUID == mNodeUUID || nodeUUID == mContractorUUID) {
                    continue;
                }
                intermediateNodes.push_back(nodeUUID);
                Path path(
                    mNodeUUID,
                    mContractorUUID,
                    intermediateNodes);
                mPathCollection->add(path);
                intermediateNodes.pop_back();
                //info() << "found path on fifth level";
            }
        }
    }
    /*Duration methodTime = utc_now() - startTime;
    info() << "findPathsOnFifthLevel method time: " << methodTime;*/
}

void PathsManager::findPathsOnSixthLevel(
    unordered_map<NodeUUID, vector<NodeUUID>, boost::hash<boost::uuids::uuid>> &contractorRT3,
    unordered_map<NodeUUID, vector<NodeUUID>, boost::hash<boost::uuids::uuid>> &contractorRT2) {

    DateTime startTime = utc_now();
    for (auto const &itRT3 : contractorRT3) {
        // TODO (mc) : need or not second condition (itRT3.first == contractorUUID)
        if (itRT3.first == mNodeUUID || itRT3.first == mContractorUUID) {
            continue;
        }
        for (auto const &nodeUUIDAndNodeUUID : mStorageHandler->routingTablesHandler()->subRoutesThirdLevelWithForbiddenNodes(
                itRT3.first,
                mNodeUUID,
                mContractorUUID)) {
            vector<NodeUUID> intermediateNodes;
            intermediateNodes.push_back(nodeUUIDAndNodeUUID.first);
            intermediateNodes.push_back(nodeUUIDAndNodeUUID.second);
            intermediateNodes.push_back(itRT3.first);
            for (auto const &nodeUUID : itRT3.second) {
                if(std::find(intermediateNodes.begin(), intermediateNodes.end(), nodeUUID) != intermediateNodes.end() ||
                   nodeUUID == mNodeUUID || nodeUUID == mContractorUUID) {
                    continue;
                }
                intermediateNodes.push_back(nodeUUID);
                for (auto &contactorIntermediateNode : intermediateNodesOnContractorFirstLevel(
                        nodeUUID,
                        intermediateNodes,
                        contractorRT2)) {

                    intermediateNodes.push_back(contactorIntermediateNode);
                    Path path(
                        mNodeUUID,
                        mContractorUUID,
                        intermediateNodes);
                    mPathCollection->add(path);
                    intermediateNodes.pop_back();

                    //info() << "found path on sixth level";
                }
                intermediateNodes.pop_back();
            }
        }
    }
    /*Duration methodTime = utc_now() - startTime;
    info() << "findPathsOnSixthLevel method time: " << methodTime;*/
}

vector<NodeUUID> PathsManager::intermediateNodesOnContractorFirstLevel(
    const NodeUUID &thirdLevelSourceNode,
    vector<NodeUUID> &intermediateNodes,
    unordered_map<NodeUUID, vector<NodeUUID>, boost::hash<boost::uuids::uuid>> &contractorRT2) const {

    auto nodeUUIDAndVect = contractorRT2.find(thirdLevelSourceNode);
    if (nodeUUIDAndVect == contractorRT2.end()) {
        return {};
    } else {
        vector<NodeUUID> result;
        for (auto const &nodeUUID : nodeUUIDAndVect->second) {
            if(std::find(intermediateNodes.begin(), intermediateNodes.end(), nodeUUID) != intermediateNodes.end() ||
                    nodeUUID == mNodeUUID || nodeUUID == mContractorUUID) {
                continue;
            }
            result.push_back(nodeUUID);
        }
        return result;
    }
}

// test
void PathsManager::findPathsOnSecondLevelTest() {

    DateTime startTime = utc_now();
    for (auto const &nodeUUID : mStorageHandler->routingTablesHandler()->subRoutesSecondLevel(mContractorUUID)) {
        Path path(
            mNodeUUID,
            mContractorUUID,
            {nodeUUID});
        if (isPathValid(path)) {
            mPathCollection->add(path);
            //info() << "found path on second level";
        }
    }
    /*Duration methodTime = utc_now() - startTime;
    info() << "findPathsOnSecondLevel test method time: " << methodTime;*/
}

void PathsManager::findPathsOnThirdLevelTest() {

    DateTime startTime = utc_now();
    for (auto const &nodeUUIDAndNodeUUID : mStorageHandler->routingTablesHandler()->subRoutesThirdLevel(
            mContractorUUID)) {
        if (nodeUUIDAndNodeUUID.first == mContractorUUID || nodeUUIDAndNodeUUID.second == mNodeUUID) {
            continue;
        }
        Path path(
            mNodeUUID,
            mContractorUUID,
            {
                nodeUUIDAndNodeUUID.first,
                nodeUUIDAndNodeUUID.second});
        if (isPathValid(path)) {
            mPathCollection->add(path);
            //info() << "found path on third level";
        }
    }
    /*Duration methodTime = utc_now() - startTime;
    info() << "findPathsOnThirdLevel test method time: " << methodTime;*/
}

void PathsManager::findPathsOnForthLevelTest(
    vector<NodeUUID> &contractorRT1) {

    DateTime startTime = utc_now();
    for (auto const &nodeUUID : contractorRT1) {
        if (nodeUUID == mNodeUUID) {
            continue;
        }
        for (auto const &nodeUUIDAndNodeUUID : mStorageHandler->routingTablesHandler()->subRoutesThirdLevel(
                nodeUUID)) {
            Path path(
                mNodeUUID,
                mContractorUUID,
                {
                    nodeUUIDAndNodeUUID.first,
                    nodeUUIDAndNodeUUID.second,
                    nodeUUID});
            if (isPathValid(path)) {
                mPathCollection->add(path);
                //info() << "found path on forth level";
            }
        }
    }
    /*Duration methodTime = utc_now() - startTime;
    info() << "findPathsOnForthLevel test method time: " << methodTime;*/
}

void PathsManager::findPathsOnFifthLevelTest(
    unordered_map<NodeUUID, vector<NodeUUID>, boost::hash<boost::uuids::uuid>> &contractorRT2) {

    DateTime startTime = utc_now();
    for (auto const &itRT2 : contractorRT2) {
        // TODO (mc) : need or not second condition (itRT2.first == contractorUUID)
        if (itRT2.first == mNodeUUID || itRT2.first == mContractorUUID) {
            continue;
        }
        for (auto const &nodeUUIDAndNodeUUID : mStorageHandler->routingTablesHandler()->subRoutesThirdLevel(
                itRT2.first)) {
            for (auto const &nodeUUID : itRT2.second) {
                vector<NodeUUID> intermediateNodes;
                intermediateNodes.push_back(nodeUUIDAndNodeUUID.first);
                intermediateNodes.push_back(nodeUUIDAndNodeUUID.second);
                intermediateNodes.push_back(itRT2.first);
                intermediateNodes.push_back(nodeUUID);
                Path path(
                    mNodeUUID,
                    mContractorUUID,
                    intermediateNodes);
                if (isPathValid(path)) {
                    mPathCollection->add(path);
                    //info() << "found path on fifth level test";
                }
            }
        }
    }
    Duration methodTime = utc_now() - startTime;
    info() << "findPathsOnFifthLevel test method time: " << methodTime;
}

void PathsManager::findPathsOnSixthLevelTest(
    unordered_map<NodeUUID, vector<NodeUUID>, boost::hash<boost::uuids::uuid>> &contractorRT3,
    unordered_map<NodeUUID, vector<NodeUUID>, boost::hash<boost::uuids::uuid>> &contractorRT2) {

    DateTime startTime = utc_now();
    for (auto const &itRT3 : contractorRT3) {
        // TODO (mc) : need or not second condition (itRT3.first == contractorUUID)
        if (itRT3.first == mNodeUUID || itRT3.first == mContractorUUID) {
            continue;
        }
        for (auto const &nodeUUIDAndNodeUUID : mStorageHandler->routingTablesHandler()->subRoutesThirdLevel(
                itRT3.first)) {

            for (auto const &nodeUUID : itRT3.second) {
                for (auto &contactorIntermediateNode : intermediateNodesOnContractorFirstLevelTest(
                        nodeUUID,
                        contractorRT2)) {
                    vector<NodeUUID> intermediateNodes;
                    intermediateNodes.push_back(nodeUUIDAndNodeUUID.first);
                    intermediateNodes.push_back(nodeUUIDAndNodeUUID.second);
                    intermediateNodes.push_back(itRT3.first);
                    intermediateNodes.push_back(nodeUUID);
                    intermediateNodes.push_back(contactorIntermediateNode);
                    Path path(
                        mNodeUUID,
                        mContractorUUID,
                        intermediateNodes);
                    if (isPathValid(path)) {
                        mPathCollection->add(path);
                        //info() << "found path on sixth level test";
                    }
                }
            }
        }
    }
    /*Duration methodTime = utc_now() - startTime;
    info() << "findPathsOnSixthLevel test method time: " << methodTime;*/
}

vector<NodeUUID> PathsManager::intermediateNodesOnContractorFirstLevelTest(
    const NodeUUID &thirdLevelSourceNode,
    unordered_map<NodeUUID, vector<NodeUUID>, boost::hash<boost::uuids::uuid>> &contractorRT2) const {

    auto nodeUUIDAndVect = contractorRT2.find(thirdLevelSourceNode);
    if (nodeUUIDAndVect == contractorRT2.end()) {
        return {};
    } else {
        return nodeUUIDAndVect->second;
    }
}

bool PathsManager::isPathValid(const Path &path) {

    auto itGlobal = path.nodes.begin();
    while (itGlobal != path.nodes.end() - 1) {
        auto itLocal = itGlobal + 1;
        while (itLocal != path.nodes.end()) {
            if (*itGlobal == *itLocal) {
                return false;
            }
            itLocal++;
        }
        itGlobal++;
    }
    return true;
}
// test end

void PathsManager::findPaths(
    const NodeUUID &contractorUUID,
    vector<NodeUUID> &contractorRT1,
    unordered_map<NodeUUID, vector<NodeUUID>, boost::hash<boost::uuids::uuid>> &contractorRT2,
    unordered_map<NodeUUID, vector<NodeUUID>, boost::hash<boost::uuids::uuid>> &contractorRT3) {

    mContractorUUID = contractorUUID;

    info() << "start finding paths to " << contractorUUID;
    DateTime startTime = utc_now();
    mPathCollection = make_shared<PathsCollection>(
        mNodeUUID,
        mContractorUUID);
    findDirectPath();
    findPathsOnSecondLevel();
    findPathsOnThirdLevel();
    findPathsOnForthLevel(
        contractorRT1);
    findPathsOnFifthLevel(
        contractorRT2);
    findPathsOnSixthLevel(
        contractorRT3,
        contractorRT2);

    Duration methodTime = utc_now() - startTime;
    info() << "PathsManager::findPath\tmethod time: " << methodTime;
    info() << "total paths count: " << mPathCollection->count();
    while (mPathCollection->hasNextPath()) {
        //info() << mPathCollection->nextPath()->toString();
        if (!isPathValid(*mPathCollection->nextPath().get())) {
            info() << "wrong path!!! ";
        }
    }
}

void PathsManager::findPathsOnSelfArea(
    const NodeUUID &contractorUUID) {

    mContractorUUID = contractorUUID;
    info() << "start finding paths on self area to " << contractorUUID;
    DateTime startTime = utc_now();
    mPathCollection = make_shared<PathsCollection>(
        mNodeUUID,
        mContractorUUID);
    findDirectPath();
    findPathsOnSecondLevel();
    findPathsOnThirdLevel();

    Duration methodTime = utc_now() - startTime;
    info() << "PathsManager::findPathsOnSelfArea\tmethod time: " << methodTime;
    info() << "total paths on self area count: " << mPathCollection->count();
    /*while (mPathCollection->hasNextPath()) {
        info() << mPathCollection->nextPath()->toString();
    }*/
}

void PathsManager::findPathsTest(
    const NodeUUID &contractorUUID,
    vector<NodeUUID> &contractorRT1,
    unordered_map<NodeUUID, vector<NodeUUID>, boost::hash<boost::uuids::uuid>> &contractorRT2,
    unordered_map<NodeUUID, vector<NodeUUID>, boost::hash<boost::uuids::uuid>> &contractorRT3) {

    mContractorUUID = contractorUUID;

    info() << "start finding test paths to " << mContractorUUID;
    DateTime startTime = utc_now();
    mPathCollection = make_shared<PathsCollection>(
        mNodeUUID,
        mContractorUUID);
    findDirectPath();
    findPathsOnSecondLevelTest();
    findPathsOnThirdLevelTest();
    findPathsOnForthLevelTest(
        contractorRT1);
    findPathsOnFifthLevelTest(
        contractorRT2);
    findPathsOnSixthLevelTest(
        contractorRT3,
        contractorRT2);

    Duration methodTime = utc_now() - startTime;
    info() << "PathsManager::findPathTest\tmethod time: " << methodTime;
    info() << "total paths test count: " << mPathCollection->count();
    /*while (mPathCollection->hasNextPath()) {
        info() << mPathCollection->nextPath()->toString();
    }*/
}

PathsCollection::Shared PathsManager::pathCollection() const {

    return mPathCollection;
}

void PathsManager::fillRoutingTables() {

    NodeUUID* nodeUUID90Ptr = new NodeUUID("13e5cf8c-5834-4e52-b65b-f9281dd1ff90");
    NodeUUID* nodeUUID91Ptr = new NodeUUID("13e5cf8c-5834-4e52-b65b-f9281dd1ff91");
    NodeUUID* nodeUUID92Ptr = new NodeUUID("13e5cf8c-5834-4e52-b65b-f9281dd1ff92");
    NodeUUID* nodeUUID93Ptr = new NodeUUID("13e5cf8c-5834-4e52-b65b-f9281dd1ff93");
    NodeUUID* nodeUUID94Ptr = new NodeUUID("13e5cf8c-5834-4e52-b65b-f9281dd1ff94");
    NodeUUID* nodeUUID95Ptr = new NodeUUID("13e5cf8c-5834-4e52-b65b-f9281dd1ff95");
    NodeUUID* nodeUUID96Ptr = new NodeUUID("13e5cf8c-5834-4e52-b65b-f9281dd1ff96");
    NodeUUID* nodeUUID97Ptr = new NodeUUID("13e5cf8c-5834-4e52-b65b-f9281dd1ff97");
    NodeUUID* nodeUUID98Ptr = new NodeUUID("13e5cf8c-5834-4e52-b65b-f9281dd1ff98");

    if (!mNodeUUID.stringUUID().compare("13e5cf8c-5834-4e52-b65b-f9281dd1ff90")) {
        mStorageHandler->routingTablesHandler()->routingTable2Level()->saveRecord(*nodeUUID92Ptr, *nodeUUID94Ptr,
                                                                              TrustLineDirection::Both);
        mStorageHandler->routingTablesHandler()->routingTable2Level()->saveRecord(*nodeUUID92Ptr, *nodeUUID90Ptr,
                                                                              TrustLineDirection::Both);
        mStorageHandler->routingTablesHandler()->routingTable2Level()->saveRecord(*nodeUUID94Ptr, *nodeUUID92Ptr,
                                                                              TrustLineDirection::Both);
        mStorageHandler->routingTablesHandler()->routingTable2Level()->saveRecord(*nodeUUID94Ptr, *nodeUUID96Ptr,
                                                                              TrustLineDirection::Both);
        mStorageHandler->routingTablesHandler()->routingTable2Level()->saveRecord(*nodeUUID94Ptr, *nodeUUID97Ptr,
                                                                              TrustLineDirection::Both);
        mStorageHandler->routingTablesHandler()->routingTable2Level()->saveRecord(*nodeUUID94Ptr, *nodeUUID98Ptr,
                                                                              TrustLineDirection::Both);
        mStorageHandler->routingTablesHandler()->routingTable2Level()->saveRecord(*nodeUUID94Ptr, *nodeUUID90Ptr,
                                                                              TrustLineDirection::Both);
        mStorageHandler->routingTablesHandler()->routingTable2Level()->commit();

        mStorageHandler->routingTablesHandler()->routingTable3Level()->saveRecord(*nodeUUID94Ptr, *nodeUUID90Ptr,
                                                                              TrustLineDirection::Both);
        mStorageHandler->routingTablesHandler()->routingTable3Level()->saveRecord(*nodeUUID94Ptr, *nodeUUID92Ptr,
                                                                              TrustLineDirection::Both);
        mStorageHandler->routingTablesHandler()->routingTable3Level()->saveRecord(*nodeUUID94Ptr, *nodeUUID96Ptr,
                                                                              TrustLineDirection::Both);
        mStorageHandler->routingTablesHandler()->routingTable3Level()->saveRecord(*nodeUUID94Ptr, *nodeUUID97Ptr,
                                                                              TrustLineDirection::Both);
        mStorageHandler->routingTablesHandler()->routingTable3Level()->saveRecord(*nodeUUID94Ptr, *nodeUUID98Ptr,
                                                                              TrustLineDirection::Both);
        mStorageHandler->routingTablesHandler()->routingTable3Level()->saveRecord(*nodeUUID96Ptr, *nodeUUID95Ptr,
                                                                              TrustLineDirection::Both);
        mStorageHandler->routingTablesHandler()->routingTable3Level()->saveRecord(*nodeUUID96Ptr, *nodeUUID94Ptr,
                                                                              TrustLineDirection::Both);
        mStorageHandler->routingTablesHandler()->routingTable3Level()->saveRecord(*nodeUUID98Ptr, *nodeUUID95Ptr,
                                                                              TrustLineDirection::Both);
        mStorageHandler->routingTablesHandler()->routingTable3Level()->saveRecord(*nodeUUID98Ptr, *nodeUUID94Ptr,
                                                                              TrustLineDirection::Both);
        mStorageHandler->routingTablesHandler()->routingTable3Level()->saveRecord(*nodeUUID97Ptr, *nodeUUID94Ptr,
                                                                              TrustLineDirection::Both);
        mStorageHandler->routingTablesHandler()->routingTable3Level()->saveRecord(*nodeUUID92Ptr, *nodeUUID90Ptr,
                                                                              TrustLineDirection::Both);
        mStorageHandler->routingTablesHandler()->routingTable3Level()->saveRecord(*nodeUUID92Ptr, *nodeUUID94Ptr,
                                                                              TrustLineDirection::Both);
        mStorageHandler->routingTablesHandler()->routingTable3Level()->commit();
    }

    if (!mNodeUUID.stringUUID().compare("13e5cf8c-5834-4e52-b65b-f9281dd1ff91")) {
        mStorageHandler->routingTablesHandler()->routingTable2Level()->saveRecord(*nodeUUID93Ptr, *nodeUUID95Ptr,
                                                                              TrustLineDirection::Both);
        mStorageHandler->routingTablesHandler()->routingTable2Level()->saveRecord(*nodeUUID93Ptr, *nodeUUID91Ptr,
                                                                              TrustLineDirection::Both);
        mStorageHandler->routingTablesHandler()->routingTable2Level()->commit();

        mStorageHandler->routingTablesHandler()->routingTable3Level()->saveRecord(*nodeUUID95Ptr, *nodeUUID96Ptr,
                                                                              TrustLineDirection::Both);
        mStorageHandler->routingTablesHandler()->routingTable3Level()->saveRecord(*nodeUUID95Ptr, *nodeUUID98Ptr,
                                                                              TrustLineDirection::Both);
        mStorageHandler->routingTablesHandler()->routingTable3Level()->saveRecord(*nodeUUID95Ptr, *nodeUUID93Ptr,
                                                                              TrustLineDirection::Both);
        mStorageHandler->routingTablesHandler()->routingTable3Level()->commit();
    }

    if (!mNodeUUID.stringUUID().compare("13e5cf8c-5834-4e52-b65b-f9281dd1ff94")) {
        mStorageHandler->routingTablesHandler()->routingTable2Level()->saveRecord(*nodeUUID92Ptr, *nodeUUID90Ptr,
                                                                              TrustLineDirection::Both);
        mStorageHandler->routingTablesHandler()->routingTable2Level()->saveRecord(*nodeUUID92Ptr, *nodeUUID94Ptr,
                                                                              TrustLineDirection::Both);
        mStorageHandler->routingTablesHandler()->routingTable2Level()->saveRecord(*nodeUUID96Ptr, *nodeUUID95Ptr,
                                                                              TrustLineDirection::Both);
        mStorageHandler->routingTablesHandler()->routingTable2Level()->saveRecord(*nodeUUID96Ptr, *nodeUUID94Ptr,
                                                                              TrustLineDirection::Both);
        mStorageHandler->routingTablesHandler()->routingTable2Level()->saveRecord(*nodeUUID98Ptr, *nodeUUID95Ptr,
                                                                              TrustLineDirection::Both);
        mStorageHandler->routingTablesHandler()->routingTable2Level()->saveRecord(*nodeUUID98Ptr, *nodeUUID94Ptr,
                                                                              TrustLineDirection::Both);
        mStorageHandler->routingTablesHandler()->routingTable2Level()->saveRecord(*nodeUUID97Ptr, *nodeUUID94Ptr,
                                                                              TrustLineDirection::Both);
        mStorageHandler->routingTablesHandler()->routingTable2Level()->commit();

        mStorageHandler->routingTablesHandler()->routingTable3Level()->saveRecord(*nodeUUID90Ptr, *nodeUUID92Ptr,
                                                                              TrustLineDirection::Both);
        mStorageHandler->routingTablesHandler()->routingTable3Level()->saveRecord(*nodeUUID90Ptr, *nodeUUID94Ptr,
                                                                              TrustLineDirection::Both);
        mStorageHandler->routingTablesHandler()->routingTable3Level()->saveRecord(*nodeUUID92Ptr, *nodeUUID90Ptr,
                                                                              TrustLineDirection::Both);
        mStorageHandler->routingTablesHandler()->routingTable3Level()->saveRecord(*nodeUUID92Ptr, *nodeUUID94Ptr,
                                                                              TrustLineDirection::Both);
        mStorageHandler->routingTablesHandler()->routingTable3Level()->saveRecord(*nodeUUID95Ptr, *nodeUUID93Ptr,
                                                                              TrustLineDirection::Both);
        mStorageHandler->routingTablesHandler()->routingTable3Level()->saveRecord(*nodeUUID95Ptr, *nodeUUID98Ptr,
                                                                              TrustLineDirection::Both);
        mStorageHandler->routingTablesHandler()->routingTable3Level()->saveRecord(*nodeUUID95Ptr, *nodeUUID96Ptr,
                                                                              TrustLineDirection::Both);
        mStorageHandler->routingTablesHandler()->routingTable3Level()->commit();
    }

    if (!mNodeUUID.stringUUID().compare("13e5cf8c-5834-4e52-b65b-f9281dd1ff93")) {
        mStorageHandler->routingTablesHandler()->routingTable2Level()->saveRecord(*nodeUUID95Ptr, *nodeUUID96Ptr,
                                                                              TrustLineDirection::Both);
        mStorageHandler->routingTablesHandler()->routingTable2Level()->saveRecord(*nodeUUID95Ptr, *nodeUUID98Ptr,
                                                                              TrustLineDirection::Both);
        mStorageHandler->routingTablesHandler()->routingTable2Level()->saveRecord(*nodeUUID95Ptr, *nodeUUID93Ptr,
                                                                              TrustLineDirection::Both);
        mStorageHandler->routingTablesHandler()->routingTable2Level()->commit();

        mStorageHandler->routingTablesHandler()->routingTable3Level()->saveRecord(*nodeUUID96Ptr, *nodeUUID94Ptr,
                                                                              TrustLineDirection::Both);
        mStorageHandler->routingTablesHandler()->routingTable3Level()->saveRecord(*nodeUUID96Ptr, *nodeUUID95Ptr,
                                                                              TrustLineDirection::Both);
        mStorageHandler->routingTablesHandler()->routingTable3Level()->saveRecord(*nodeUUID98Ptr, *nodeUUID94Ptr,
                                                                              TrustLineDirection::Both);
        mStorageHandler->routingTablesHandler()->routingTable3Level()->saveRecord(*nodeUUID98Ptr, *nodeUUID95Ptr,
                                                                              TrustLineDirection::Both);
        mStorageHandler->routingTablesHandler()->routingTable3Level()->commit();
    }

    delete nodeUUID90Ptr;
    delete nodeUUID91Ptr;
    delete nodeUUID92Ptr;
    delete nodeUUID93Ptr;
    delete nodeUUID94Ptr;
    delete nodeUUID95Ptr;
    delete nodeUUID96Ptr;
    delete nodeUUID97Ptr;
    delete nodeUUID98Ptr;
}

void PathsManager::testStorageHandler() {

    NodeUUID* nodeUUID81Ptr = new NodeUUID("13e5cf8c-5834-4e52-b65b-f9281dd1ff81");
    NodeUUID* nodeUUID82Ptr = new NodeUUID("13e5cf8c-5834-4e52-b65b-f9281dd1ff82");
    NodeUUID* nodeUUID83Ptr = new NodeUUID("13e5cf8c-5834-4e52-b65b-f9281dd1ff83");
    NodeUUID* nodeUUID84Ptr = new NodeUUID("13e5cf8c-5834-4e52-b65b-f9281dd1ff84");
    NodeUUID* nodeUUID85Ptr = new NodeUUID("13e5cf8c-5834-4e52-b65b-f9281dd1ff85");
    NodeUUID* nodeUUID86Ptr = new NodeUUID("13e5cf8c-5834-4e52-b65b-f9281dd1ff86");
    mStorageHandler->routingTablesHandler()->routingTable2Level()->saveRecord(*nodeUUID81Ptr, *nodeUUID82Ptr, TrustLineDirection::Both);
    mStorageHandler->routingTablesHandler()->routingTable2Level()->rollBack();
    mStorageHandler->routingTablesHandler()->routingTable2Level()->saveRecord(*nodeUUID81Ptr, *nodeUUID82Ptr, TrustLineDirection::Both);
    mStorageHandler->routingTablesHandler()->routingTable2Level()->commit();
    mStorageHandler->routingTablesHandler()->routingTable2Level()->saveRecord(*nodeUUID83Ptr, *nodeUUID84Ptr, TrustLineDirection::Incoming);
    mStorageHandler->routingTablesHandler()->routingTable2Level()->saveRecord(*nodeUUID85Ptr, *nodeUUID86Ptr, TrustLineDirection::Outgoing);
    mStorageHandler->routingTablesHandler()->routingTable2Level()->commit();
    mStorageHandler->routingTablesHandler()->routingTable2Level()->saveRecord(*nodeUUID82Ptr, *nodeUUID81Ptr, TrustLineDirection::Both);
    mStorageHandler->routingTablesHandler()->routingTable2Level()->rollBack();
    mStorageHandler->routingTablesHandler()->routingTable2Level()->commit();
    mStorageHandler->routingTablesHandler()->routingTable2Level()->saveRecord(*nodeUUID81Ptr, *nodeUUID83Ptr, TrustLineDirection::Incoming);
    mStorageHandler->routingTablesHandler()->routingTable2Level()->saveRecord(*nodeUUID81Ptr, *nodeUUID84Ptr, TrustLineDirection::Both);
    mStorageHandler->routingTablesHandler()->routingTable2Level()->saveRecord(*nodeUUID83Ptr, *nodeUUID81Ptr, TrustLineDirection::Outgoing);
    mStorageHandler->routingTablesHandler()->routingTable2Level()->commit();


    vector<tuple<NodeUUID, NodeUUID, TrustLineDirection>> records = mStorageHandler->routingTablesHandler()->routingTable2Level()->routeRecordsWithDirections();
    NodeUUID source;
    NodeUUID target;
    TrustLineDirection direction;
    for (auto &record : records) {
        std::tie(source, target, direction) = record;
        info() << source << " " << target << " " << direction;
    }

    for (auto const &itMap : mStorageHandler->routingTablesHandler()->routingTable2Level()->routeRecordsWithDirectionsMapSourceKey()) {
        info() << "key: " << itMap.first;
        for (auto const &itVector : itMap.second) {
            info() << "\tvalue: " << itVector.first << " " << itVector.second;
        }
    }

    mStorageHandler->routingTablesHandler()->routingTable2Level()->deleteRecord(*nodeUUID81Ptr, *nodeUUID83Ptr);
    mStorageHandler->routingTablesHandler()->routingTable2Level()->saveRecord(*nodeUUID81Ptr, *nodeUUID84Ptr, TrustLineDirection::Outgoing);
    info() << "after updating and deleting";
    for (auto const &itMap : mStorageHandler->routingTablesHandler()->routingTable2Level()->routeRecordsWithDirectionsMapSourceKey()) {
        info() << "key: " << itMap.first;
        for (auto const &itVector : itMap.second) {
            info() << "\tvalue: " << itVector.first << " " << itVector.second;
        }
    }

    mStorageHandler->routingTablesHandler()->routingTable2Level()->saveRecord(*nodeUUID85Ptr, *nodeUUID81Ptr, TrustLineDirection::Outgoing);
    info() << "after updating absent elemnet";
    for (auto const &itMap : mStorageHandler->routingTablesHandler()->routingTable2Level()->routeRecordsWithDirectionsMapSourceKey()) {
        info() << "key: " << itMap.first;
        for (auto const &itVector : itMap.second) {
            info() << "\tvalue: " << itVector.first << " " << itVector.second;
        }
    }

    mStorageHandler->routingTablesHandler()->routingTable2Level()->saveRecord(*nodeUUID81Ptr, *nodeUUID82Ptr, TrustLineDirection::Outgoing);
    info() << "after updating real elemnet";
    for (auto const &itMap : mStorageHandler->routingTablesHandler()->routingTable2Level()->routeRecordsWithDirectionsMapSourceKey()) {
        info() << "key: " << itMap.first;
        for (auto const &itVector : itMap.second) {
            info() << "\tvalue: " << itVector.first << " " << itVector.second;
        }
    }
    mStorageHandler->routingTablesHandler()->routingTable2Level()->commit();

    info() << "all destinations for source: " << *nodeUUID81Ptr;
    for (const auto &itVect : mStorageHandler->routingTablesHandler()->routingTable2Level()->allDestinationsForSource(*nodeUUID81Ptr)) {
        info() << "\t\t\t" << itVect;
    }

    delete nodeUUID81Ptr;
    delete nodeUUID82Ptr;
    delete nodeUUID83Ptr;
    delete nodeUUID84Ptr;
    delete nodeUUID85Ptr;
    delete nodeUUID86Ptr;

}

void PathsManager::testTrustLineHandler() {

    for (auto const &trustLine : mTrustLinesManager->trustLines()) {
        mStorageHandler->trustLineHandler()->saveTrustLine(trustLine.second);
    }
    TrustLine::Shared trLine = make_shared<TrustLine>(
        mNodeUUID,
        TrustLineAmount(100),
        TrustLineAmount(200),
        TrustLineBalance(-30));
    mStorageHandler->trustLineHandler()->saveTrustLine(trLine);
    mStorageHandler->trustLineHandler()->commit();
    for (const auto &rtrLine : mStorageHandler->trustLineHandler()->trustLines()) {
        info() << "read one trust line: " <<
               rtrLine->contractorNodeUUID() << " " <<
               rtrLine->incomingTrustAmount() << " " <<
               rtrLine->outgoingTrustAmount() << " " <<
               rtrLine->balance();
    }
    trLine->setBalance(55);
    trLine->setIncomingTrustAmount(1000);
    mStorageHandler->trustLineHandler()->saveTrustLine(trLine);
    for (const auto &rtrLine : mStorageHandler->trustLineHandler()->trustLines()) {
        info() << "read one trust line: " <<
               rtrLine->contractorNodeUUID() << " " <<
               rtrLine->incomingTrustAmount() << " " <<
               rtrLine->outgoingTrustAmount() << " " <<
               rtrLine->balance();
    }
    mStorageHandler->trustLineHandler()->rollBack();
    for (const auto &rtrLine : mStorageHandler->trustLineHandler()->trustLines()) {
        info() << "read one trust line: " <<
               rtrLine->contractorNodeUUID() << " " <<
               rtrLine->incomingTrustAmount() << " " <<
               rtrLine->outgoingTrustAmount() << " " <<
               rtrLine->balance();
    }
    mStorageHandler->trustLineHandler()->deleteTrustLine(mNodeUUID);
    for (const auto &rtrLine : mStorageHandler->trustLineHandler()->trustLines()) {
        info() << "read one trust line: " <<
               rtrLine->contractorNodeUUID() << " " <<
               rtrLine->incomingTrustAmount() << " " <<
               rtrLine->outgoingTrustAmount() << " " <<
               rtrLine->balance();
    }
    mStorageHandler->trustLineHandler()->commit();
}

void PathsManager::testPaymentStateOperationsHandler() {
    TransactionUUID transaction1;
    BytesShared state1 = tryMalloc(sizeof(uint8_t));
    uint8_t st1 = 2;
    memcpy(
        state1.get(),
        &st1,
        sizeof(uint8_t));
    mStorageHandler->paymentOperationStateHandler()->saveRecord(transaction1, state1, sizeof(uint8_t));
    st1 = 22;
    memcpy(
        state1.get(),
        &st1,
        sizeof(uint8_t));
    mStorageHandler->paymentOperationStateHandler()->saveRecord(transaction1, state1, sizeof(uint8_t));
    TransactionUUID transaction2;
    BytesShared state2 = tryMalloc(sizeof(uint16_t));
    uint16_t st2 = 88;
    memcpy(
        state2.get(),
        &st2,
        sizeof(uint16_t));
    mStorageHandler->paymentOperationStateHandler()->saveRecord(transaction2, state2, sizeof(uint16_t));
    mStorageHandler->paymentOperationStateHandler()->commit();
    TransactionUUID transaction3;
    BytesShared state3 = tryMalloc(sizeof(uint32_t));
    uint32_t st3 = 3;
    memcpy(
        state3.get(),
        &st3,
        sizeof(uint32_t));
    mStorageHandler->paymentOperationStateHandler()->saveRecord(transaction3, state3, sizeof(uint32_t));
    mStorageHandler->paymentOperationStateHandler()->rollBack();

    pair<BytesShared, size_t> stateBt = mStorageHandler->paymentOperationStateHandler()->getState(transaction1);
    uint32_t state = 0;
    memcpy(
        &state,
        stateBt.first.get(),
        stateBt.second);
    info() << stateBt.second << " " << (uint32_t)state;
    try {
        stateBt = mStorageHandler->paymentOperationStateHandler()->getState(transaction2);
        memcpy(
            &state,
            stateBt.first.get(),
            stateBt.second);
        info() << stateBt.second << " " << state;
    } catch (NotFoundError) {
        info() << "not found";
    }
    try {
        stateBt = mStorageHandler->paymentOperationStateHandler()->getState(transaction3);
        memcpy(
            &state,
            stateBt.first.get(),
            stateBt.second);
        info() << stateBt.second << " " << state;
    } catch (NotFoundError) {
        info() << "not found";
    }
    state1 = tryMalloc(sizeof(uint32_t));
    uint32_t st1_1 = 101;
    memcpy(
        state1.get(),
        &st1_1,
        sizeof(uint32_t));
    mStorageHandler->paymentOperationStateHandler()->saveRecord(transaction1, state1, sizeof(uint32_t));
    mStorageHandler->paymentOperationStateHandler()->saveRecord(transaction3, state3, sizeof(uint32_t));
    mStorageHandler->paymentOperationStateHandler()->deleteRecord(transaction2);
    mStorageHandler->paymentOperationStateHandler()->commit();

    info() << "after changes";
    stateBt = mStorageHandler->paymentOperationStateHandler()->getState(transaction1);
    memcpy(
        &state,
        stateBt.first.get(),
        stateBt.second);
    info() << stateBt.second << " " << state;
    try {
        stateBt = mStorageHandler->paymentOperationStateHandler()->getState(transaction2);
        memcpy(
            &state,
            stateBt.first.get(),
            stateBt.second);
        info() << stateBt.second << " " << state;
    } catch (NotFoundError) {
        info() << "not found";
    }
    try {
        stateBt = mStorageHandler->paymentOperationStateHandler()->getState(transaction3);
        memcpy(
            &state,
            stateBt.first.get(),
            stateBt.second);
        info() << stateBt.second << " " << state;
    } catch (NotFoundError) {
        info() << "not found";
    }
    try {
        stateBt = mStorageHandler->paymentOperationStateHandler()->getState(TransactionUUID());
        memcpy(
            &state,
            stateBt.first.get(),
            stateBt.second);
        info() << stateBt.second << " " << state;
    } catch (NotFoundError) {
        info() << "not found";
    }
}

void PathsManager::fillBigRoutingTables() {

    uint32_t firstLevelNode = 10;
    uint32_t countNodes = 800;
    srand (time(NULL));
    vector<NodeUUID*> nodeUUIDPtrs;
    nodeUUIDPtrs.reserve(countNodes);
    for (uint32_t idx = 1; idx <= countNodes; idx++) {
        nodeUUIDPtrs.push_back(new NodeUUID(nodeUUIDName(idx)));
    }

    uint32_t currentIdx = 0;
    if (!mNodeUUID.stringUUID().compare("13e5cf8c-5834-4e52-b65b-000000000001")) {
        currentIdx = 1;
    }

    if (!mNodeUUID.stringUUID().compare("13e5cf8c-5834-4e52-b65b-000000000002")) {
        currentIdx = 2;
    }

    vector<uint32_t> firstLevelNodes;
    while (firstLevelNodes.size() < firstLevelNode) {
        uint32_t  nextIdx = rand() % countNodes + 1;
        if(std::find(firstLevelNodes.begin(), firstLevelNodes.end(), nextIdx) != firstLevelNodes.end() ||
           nextIdx == currentIdx) {
            continue;
        }
        firstLevelNodes.push_back(nextIdx);
    }
    info() << "PathsManager::fillBigRoutingTables first level done";
    set<uint32_t> secondLevelAllNodes;
    for (auto firstLevelIdx : firstLevelNodes) {
        vector<uint32_t> secondLevelNodes;
        while (secondLevelNodes.size() < firstLevelNode) {
            uint32_t  nextIdx = rand() % countNodes + 1;
            if(std::find(secondLevelNodes.begin(), secondLevelNodes.end(), nextIdx) != secondLevelNodes.end() ||
               nextIdx == firstLevelIdx) {
                continue;
            }
            secondLevelNodes.push_back(nextIdx);
            secondLevelAllNodes.insert(nextIdx);
            mStorageHandler->routingTablesHandler()->routingTable2Level()->saveRecord(
                *getPtrByNodeNumber(
                    firstLevelIdx,
                    nodeUUIDPtrs),
                *getPtrByNodeNumber(
                    nextIdx,
                    nodeUUIDPtrs),
                TrustLineDirection::Both);
        }
        mStorageHandler->routingTablesHandler()->routingTable2Level()->commit();
        //info() << "fillBigRoutingTables:: second level commit";
    }
    info() << "fillBigRoutingTables:: second level done";
    for (auto secondLevelIdx : secondLevelAllNodes) {
        vector<uint32_t> thirdLevelNodes;
        while (thirdLevelNodes.size() < firstLevelNode) {
            uint32_t  nextIdx = rand() % countNodes + 1;
            if(std::find(thirdLevelNodes.begin(), thirdLevelNodes.end(), nextIdx) != thirdLevelNodes.end() ||
               nextIdx == secondLevelIdx) {
                continue;
            }
            thirdLevelNodes.push_back(nextIdx);
            mStorageHandler->routingTablesHandler()->routingTable3Level()->saveRecord(
                *getPtrByNodeNumber(
                    secondLevelIdx,
                    nodeUUIDPtrs),
                *getPtrByNodeNumber(
                    nextIdx,
                    nodeUUIDPtrs),
                TrustLineDirection::Both);
        }
        mStorageHandler->routingTablesHandler()->routingTable3Level()->commit();
        //info() << "fillBigRoutingTables:: third level commit";
    }
    info() << "fillBigRoutingTables:: third level done";
    for (auto nodeUUIDPrt : nodeUUIDPtrs) {
        delete nodeUUIDPrt;
    }
}

string PathsManager::nodeUUIDName(uint32_t number) {
    stringstream s;
    s << number;
    string numStr = s.str();
    while (numStr.length() < 12) {
        numStr = "0" + numStr;
    }
    return "13e5cf8c-5834-4e52-b65b-" + numStr;
}

NodeUUID* PathsManager::getPtrByNodeNumber(
    uint32_t number,
    vector<NodeUUID*> nodeUUIDPtrs) {

    string nodeUUIDStr = nodeUUIDName(number);
    for (auto nodeUUIDPtr : nodeUUIDPtrs) {
        if (nodeUUIDStr.compare(nodeUUIDPtr->stringUUID()) == 0) {
            return nodeUUIDPtr;
        }
    }
}

void PathsManager::testTime() {

    info() << "testTime\t" << "RT2 size: " << mStorageHandler->routingTablesHandler()->routingTable2Level()->routeRecords().size();
    info() << "testTime\t" << "RT2 with directions size: " << mStorageHandler->routingTablesHandler()->routingTable2Level()->routeRecordsWithDirections().size();
    info() << "testTime\t" << "RT2 map size opt5: " << mStorageHandler->routingTablesHandler()->routingTable2Level()->routeRecordsMapDestinationKey().size();

    info() << "testTime\t" << "RT3 size: " << mStorageHandler->routingTablesHandler()->routingTable3Level()->routeRecords().size();
    info() << "testTime\t" << "RT3 with directions size: " << mStorageHandler->routingTablesHandler()->routingTable3Level()->routeRecordsWithDirections().size();
    info() << "testTime\t" << "RT3 map size opt5: " << mStorageHandler->routingTablesHandler()->routingTable3Level()->routeRecordsMapDestinationKey().size();
}

LoggerStream PathsManager::info() const {

    if (nullptr == mLog)
        throw Exception("logger is not initialised");

    return mLog->info(logHeader());
}

const string PathsManager::logHeader() const {

    stringstream s;
    s << "[PathsManager]";

    return s.str();
}
