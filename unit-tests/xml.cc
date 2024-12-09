#include <gtest/gtest.h>

#include "nova/xml.hh"

constexpr auto data = R"xml(
<foo>
    <bar>
    </bar>
</foo>
)xml";

TEST(ph, ph) {
    nova::xml doc(data);

    constexpr auto ret = R"xml(<?xml version="1.0" encoding="UTF-8"?>
<foo>
    <bar>
    </bar>
</foo>
)xml";

    ASSERT_EQ(doc.string(), ret);
}
