#ifndef ORCA_PARAM_H
#define ORCA_PARAM_H

#include "../plugin.h"

namespace Vipster::IO{

struct OrcaParam: BaseParam{
    using Header = std::vector<std::string>;
    OrcaParam(std::string="", Header={});
    Header header;
    std::unique_ptr<BaseParam> copy() override;
    void parseJson(const nlohmann::json::iterator&) override;
    nlohmann::json toJson() override;
};

void to_json(nlohmann::json& j, const OrcaParam& p);
void from_json(const nlohmann::json& j, OrcaParam& p);

}

#endif // ORCA_PARAM_H
