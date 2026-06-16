#include "DasherTypes.h"
#include <array>
#include <algorithm>

namespace Dasher {

namespace Keys {
static std::array<std::string, 104> keyNames = {
    "Invalid",   "Big_Start_Stop", "Button_1",  "Button_2",  "Button_3",  "Button_4",  "Button_5",  "Button_6",
    "Button_7",  "Button_8",       "Button_9",  "Button_10", "Button_11", "Button_12", "Button_13", "Button_14",
    "Button_15", "Button_16",      "Button_17", "Button_18", "Button_19", "Button_20", "Button_21", "Button_22",
    "Button_23", "Button_24",      "Button_25", "Button_26", "Button_27", "Button_28", "Button_29", "Button_30",
    "Button_31", "Button_32",      "Button_33", "Button_34", "Button_35", "Button_36", "Button_37", "Button_38",
    "Button_39", "Button_40",      "Button_41", "Button_42", "Button_43", "Button_44", "Button_45", "Button_46",
    "Button_47", "Button_48",      "Button_49", "Button_50", "Button_51", "Button_52", "Button_53", "Button_54",
    "Button_55", "Button_56",      "Button_57", "Button_58", "Button_59", "Button_60", "Button_61", "Button_62",
    "Button_63", "Button_64",      "Button_65", "Button_66", "Button_67", "Button_68", "Button_69", "Button_70",
    "Button_71", "Button_72",      "Button_73", "Button_74", "Button_75", "Button_76", "Button_77", "Button_78",
    "Button_79", "Button_80",      "Button_81", "Button_82", "Button_83", "Button_84", "Button_85", "Button_86",
    "Button_87", "Button_88",      "Button_89", "Button_90", "Button_91", "Button_92", "Button_93", "Button_94",
    "Button_95", "Button_96",      "Button_97", "Button_98", "Button_99", "Primary",   "Secondary", "Tertiary"};
const std::string& VirtualKeyToString(VirtualKey key) {
    return keyNames[key + 1];
};
VirtualKey StringToVirtualKey(const std::string& name) {
    auto elem = std::find(keyNames.begin(), keyNames.end(), name);
    if (elem == keyNames.end()) return Invalid_Key;
    return static_cast<VirtualKey>(std::distance(keyNames.begin(), elem) - 1);
};
} // namespace Keys
} // namespace Dasher