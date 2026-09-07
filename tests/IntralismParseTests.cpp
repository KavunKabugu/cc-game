#include <cassert>
#include <string>
#include <string_view>
#include <vector>

#include "IntralismParse.h"

namespace {

using MapConverter::IntralismDirectionToLane;
using MapConverter::ParseSpawnObjLanes;

void TestDirectionToLane() {
    assert(IntralismDirectionToLane("Left") == 0);
    assert(IntralismDirectionToLane("Up") == 1);
    assert(IntralismDirectionToLane("Right") == 2);
    assert(IntralismDirectionToLane("Down") == 3);
    assert(IntralismDirectionToLane("  Up  ") == 1);
    assert(!IntralismDirectionToLane("Foo").has_value());
    assert(!IntralismDirectionToLane("").has_value());
    assert(!IntralismDirectionToLane("left").has_value());
}

void ExpectLanes(const std::string_view payload, const std::vector<int>& expected) {
    const std::vector<int> lanes = ParseSpawnObjLanes(payload);
    assert(lanes == expected);
}

void TestSpawnObjPayloads() {
    ExpectLanes("[Up,False]", {1});
    ExpectLanes("[Left-Right,False]", {0, 2});
    ExpectLanes("[Left-Up]", {0, 1});
    ExpectLanes("[Left-Up-Down-Right,False]", {0, 1, 3, 2});
    ExpectLanes("[Down,False]", {3});
    ExpectLanes("[Right,False]", {2});
}

void TestUnknownDirectionIgnored() {
    ExpectLanes("[Left-Foo-Up]", {0, 1});
    ExpectLanes("[Nope,False]", {});
}

void TestEmptySpawnObjPayload() {
    ExpectLanes("", {});
    ExpectLanes("[]", {});
    ExpectLanes("Up,False", {});
    ExpectLanes("[,False]", {});
}

} // namespace

int main() {
    TestDirectionToLane();
    TestSpawnObjPayloads();
    TestUnknownDirectionIgnored();
    TestEmptySpawnObjPayload();
    return 0;
}
