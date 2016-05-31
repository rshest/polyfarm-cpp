#include "CppUnitTest.h"
#include <iostream>

#include <shape.hpp>


using namespace Microsoft::VisualStudio::CppUnitTestFramework;


typedef std::vector<vec2i> int_pair_vec;

namespace Microsoft {namespace VisualStudio {namespace CppUnitTestFramework {
    template<> static std::wstring ToString<int_pair_vec>(const int_pair_vec& arr) {
        std::wstringstream ws;
        for (const auto& c : arr) ws << "[" << c.x << " " << c.y << "] ";
        return ws.str();
    }

    template<> static std::wstring ToString<overlap>(const overlap& x) {
        if (x == overlap::Overlap   ) return L"Overlap";
        if (x == overlap::Border    ) return L"Border";
        if (x == overlap::Disjoint  ) return L"Disjoint";
        return L"ERROR";
    }
}}}

namespace test
{

static const char* SHAPE1 = 
    "   \n"
    " O\n"
    " OOO \n"
    " O\n"
    ;

static const char* SHAPE2 = 
    "   O\n"
    " OOO \n"
    " O\n"
    ;

static const char* SHAPE3 = 
    "OOOO\n"
    "   O\n"
    ;


TEST_CLASS(test_shape)
{
public:

    shape shape1, shape2, shape3;

    test_shape() {
        shape::parse(std::stringstream(SHAPE1), shape1);
        shape::parse(std::stringstream(SHAPE2), shape2);
        shape::parse(std::stringstream(SHAPE3), shape3);
    }

    TEST_METHOD(test_shape_parse) {
        shape sh1, sh2;

        bool res1 = shape::parse(std::stringstream(SHAPE1), sh1);
        bool res2 = shape::parse(std::stringstream(SHAPE2), sh2);

        Assert::IsTrue(res1);
        Assert::IsTrue(res2);

        std::vector<vec2i> sq1 = {{1, 1}, {1, 2}, {2, 2}, {3, 2}, {1, 3}};
        std::vector<vec2i> sq2 = {{3, 0}, {1, 1}, {2, 1}, {3, 1}, {1, 2}};

        Assert::AreEqual(sq1, sh1.squares);
        Assert::AreEqual(sq2, sh2.squares);
    }

    TEST_METHOD(test_shape_rotated) {
        shape sh0 = shape3.rotated(rotation::None);
        std::vector<vec2i> sq0 = {{0, 0}, {1, 0}, {2, 0}, {3, 0}, {3, 1}};
        Assert::AreEqual(sq0, sh0.squares);
        Assert::AreEqual(shape3.width,  sh0.width);
        Assert::AreEqual(shape3.height, sh0.height);
        
        shape sh1 = shape3.rotated(rotation::CW_90);
        std::vector<vec2i> sq1 = {{1, 0}, {1, 1}, {1, 2}, {1, 3}, {0, 3}};
        Assert::AreEqual(sq1, sh1.squares);
        Assert::AreEqual(shape3.width,  sh1.height);
        Assert::AreEqual(shape3.height, sh1.width);

        shape sh2 = shape3.rotated(rotation::CW_180);
        std::vector<vec2i> sq2 = {{3, 1}, {2, 1}, {1, 1}, {0, 1}, {0, 0}};
        Assert::AreEqual(sq2, sh2.squares);
        Assert::AreEqual(shape3.width,  sh2.width);
        Assert::AreEqual(shape3.height, sh2.height);

        shape sh3 = shape3.rotated(rotation::CW_270);
        std::vector<vec2i> sq3 = {{0, 3}, {0, 2}, {0, 1}, {0, 0}, {1, 0}};
        Assert::AreEqual(sq3, sh3.squares);
        Assert::AreEqual(shape3.width,  sh3.height);
        Assert::AreEqual(shape3.height, sh3.width);

    }

    TEST_METHOD(test_overlap_status) {
        Assert::AreEqual(overlap::Overlap, 
            overlap_status(shape1, {0, 0}, shape2, {0, 0}));

        Assert::AreEqual(overlap::Border, 
            overlap_status(shape1, {0, 0}, shape2, {0, 3}));

        Assert::AreEqual(overlap::Border, 
            overlap_status(shape1, {1, 1}, shape2, {1, 4}));

        Assert::AreEqual(overlap::Disjoint, 
            overlap_status(shape1, {0, 0}, shape2, {0, 4}));

        Assert::AreEqual(overlap::Border, 
            overlap_status(shape2, {-2, 0}, shape3, {0, 2}));
    }

    TEST_METHOD(test_distance) {
        Assert::AreEqual(-1, 
            distance(shape2, {0, 0}, shape3, {0, 0}));

        Assert::AreEqual(0, 
            distance(shape2, {0, -3}, shape3, {0, 0}));
        
        Assert::AreEqual(1, 
            distance(shape2, {0, -4}, shape3, {0, 0}));

        Assert::AreEqual(2, 
            distance(shape2, {0, -4}, shape3, {1, 1}));
    }

    TEST_METHOD(test_angle_greater) {
        Assert::IsTrue(angle_greater(1.0, 0.0));
        Assert::IsFalse(angle_greater(0.0, 0.0));
        Assert::IsFalse(angle_greater(0.0, 1.0));
        
        Assert::IsTrue(angle_greater(2.0, 1.0));
        Assert::IsTrue(angle_greater(1.0, 6.0));
        Assert::IsFalse(angle_greater(6.0, 1.0));
    }

};

}