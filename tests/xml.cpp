#include "serde/xml/format.hpp"

#include "macro.hpp"

#include "enum.hpp"

// attributes
struct Attributes {
    SerdeFieldsBegin;
    std::string SerdeField(string);
    int         SerdeField(integer);
    double      SerdeField(floating);
    Enum        SerdeField(enumerator);
    SerdeFieldsEnd;
};

auto attributes() -> int {
    const auto str = R"(
        <root string="string" integer="1" floating="2.0" enumerator="a"/>
    )";

    unwrap(node_pre, xml::parse(str));
    unwrap(obj_pre, (serde::load<serde::XmlFormat, Attributes>(node_pre)));
    unwrap(node, obj_pre.dump<serde::XmlFormat>());
    unwrap(obj, (serde::load<serde::XmlFormat, Attributes>(node)));

    ensure(obj.string == "string");
    ensure(obj.integer == 1);
    ensure(obj.floating == 2.0);
    ensure(obj.enumerator == Enum::A);
    return 0;
}

// data
struct Data {
    SerdeFieldsBegin;
    SerdeFieldsEnd;
    std::string data;
};

auto data() -> int {
    const auto str = R"(
        <root>hello, world!</root>
    )";

    unwrap(node_pre, xml::parse(str));
    unwrap(obj_pre, (serde::load<serde::XmlFormat, Data>(node_pre)));
    unwrap(node, obj_pre.dump<serde::XmlFormat>());
    unwrap(obj, (serde::load<serde::XmlFormat, Data>(node)));

    ensure(obj.data == "hello, world!");
    return 0;
}

// child elements
struct GrandChild {
    SerdeFieldsBegin;
    std::string SerdeField(name);
    SerdeFieldsEnd;
};

struct Child {
    SerdeFieldsBegin;
    std::vector<GrandChild> SerdeNamedField(grand_children, "grand-child");
    SerdeFieldsEnd;
};

struct Elements {
    SerdeFieldsBegin;
    std::vector<Child>   SerdeNamedField(children, "child");
    std::array<Child, 3> SerdeNamedField(triple, "triple");
    SerdeFieldsEnd;
};

auto elements() -> int {
    const auto str = R"(
        <root>
            <child>
                <grand-child name="a1"/>
            </child>
            <child>
                <grand-child name="b1"/>
                <grand-child name="b2"/>
            </child>

            <triple/>
            <triple/>
            <triple/>
        </root>
    )";

    unwrap(node_pre, xml::parse(str));
    unwrap(obj_pre, (serde::load<serde::XmlFormat, Elements>(node_pre)));
    unwrap(node, obj_pre.dump<serde::XmlFormat>());
    unwrap(obj, (serde::load<serde::XmlFormat, Elements>(node)));

    ensure(obj.children.size() == 2);
    ensure(obj.children[0].grand_children.size() == 1);
    ensure(obj.children[0].grand_children[0].name == "a1");
    ensure(obj.children[1].grand_children.size() == 2);
    ensure(obj.children[1].grand_children[0].name == "b1");
    ensure(obj.children[1].grand_children[1].name == "b2");
    return 0;
}

// serde features
struct Features {
    SerdeFieldsBegin;
    std::optional<int> SerdeField(onum1);       // optional not exists
    std::optional<int> SerdeField(onum2);       // optional exists
    int                SerdeField(num_def, -1); // default value
    SerdeFieldsEnd;
};

auto features() -> int {
    const auto str = R"(
        <root onum2="2" num_def="3"/>
    )";

    unwrap(node_pre, xml::parse(str));
    unwrap(obj_pre, (serde::load<serde::XmlFormat, Features>(node_pre)));
    unwrap(node, obj_pre.dump<serde::XmlFormat>());
    unwrap(obj, (serde::load<serde::XmlFormat, Features>(node)));

    ensure(!obj.onum1.has_value());
    ensure(obj.onum2.has_value() && obj.onum2.value() == 2);
    ensure(Features().num_def == -1);
    ensure(obj.num_def == 3);
    return 0;
}

// dump/load to existing object
struct NonSerdeField {
    SerdeFieldsBegin;
    SerdeFieldsEnd;
    int dont_care;
};

auto non_serde_field() -> int {
    const auto str = R"(
        <root num1="1" num2="2" />
    )";

    unwrap(node_pre, xml::parse(str));
    unwrap(obj_pre, (serde::load<serde::XmlFormat, NonSerdeField>(node_pre)));
    unwrap(node, obj_pre.dump<serde::XmlFormat>(xml::Node{.name = "fakeroot", .attrs = {{"dont-care", "8"}}}));
    unwrap(obj, (serde::load<serde::XmlFormat, NonSerdeField>(node, NonSerdeField{.dont_care = 9})));

    ensure(node.name == "fakeroot");
    ensure(node.is_attr_equal("dont-care", "8"));
    ensure(obj.dont_care = 9);
    return 0;
}

// missing field
struct MissingField {
    SerdeFieldsBegin;
    int SerdeField(a);
    int SerdeField(b);
    int SerdeField(c);
    SerdeFieldsEnd;
};

auto missing_field() -> int {
    const auto str = R"(
        <root a="1" c="1"/>
    )";

    unwrap(node, xml::parse(str));
    ensure(!(serde::load<serde::XmlFormat, MissingField>(node)).has_value());
    return 0;
}

// mismatched array length
struct MismatchedChildrenLength {
    SerdeFieldsBegin;
    std::array<GrandChild, 3> SerdeField(e);
    SerdeFieldsEnd;
};

auto mismatched_children_length() -> int {
    const auto str = R"(
        <root>
            <e name="1"/>
            <e name="2"/>
            <e name="3"/>
            <e name="4"/>
        </root>
    )";

    unwrap(node, xml::parse(str));
    ensure(!(serde::load<serde::XmlFormat, MismatchedChildrenLength>(node)).has_value());
    return 0;
}

auto main() -> int {
    ensure(attributes() == 0);
    ensure(elements() == 0);
    ensure(features() == 0);
    ensure(non_serde_field() == 0);
    ensure(missing_field() == 0);
    ensure(mismatched_children_length() == 0);
    std::println("pass");
    return 0;
}
