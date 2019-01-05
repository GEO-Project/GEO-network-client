#ifndef GEO_NETWORK_CLIENT_SETTINGS_H
#define GEO_NETWORK_CLIENT_SETTINGS_H

#include "../common/Types.h"

#include "../common/exceptions/IOError.h"
#include "../common/exceptions/RuntimeError.h"

#include "../../libs/json/json.h"

#include <string>
#include <fstream>
#include <streambuf>

using namespace std;
using json = nlohmann::json;

class Settings {
public:
    vector<pair<string, string>> addresses(
        const json *conf = nullptr) const;

    vector<SerializedEquivalent> iAmGateway(
        const json *conf = nullptr) const;

    json loadParsedJSON() const;
};

#endif //GEO_NETWORK_CLIENT_SETTINGS_H