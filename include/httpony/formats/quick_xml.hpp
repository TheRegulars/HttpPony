/**
 * \file
 *
 * \author Mattia Basaglia
 *
 * \copyright Copyright 2016 Mattia Basaglia
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef HTTPONY_QUICK_XML_HPP
#define HTTPONY_QUICK_XML_HPP

/// \cond
#include <ostream>
#include <vector>
#include <type_traits>

#include <melanolib/string/stringutils.hpp>
#include <melanolib/utils/c++-compat.hpp>
/// \endcond

namespace httpony {
namespace quick_xml {

inline std::string amp_escape(const std::string& string)
{
    static const std::unordered_map<std::string, std::string> replacements{
        {"\"", "&quot;"},
        {"'", "&apos;"},
        {"&", "&amp;"},
        {"<", "&lt;"},
        {">", "&gt;"}
    };
    return melanolib::string::replace(string, replacements);
}

class Indentation
{
public:
    enum NodeType
    {
        Nothing     = 0x0,
        Element     = 0x1,
        Attribute   = 0x2,
        Comment     = 0x4,
        CommentText = 0x8,
    };

    using NodeTypes = std::underlying_type_t<NodeType>;

    explicit Indentation(
        NodeTypes what = Nothing,
        int  depth = 4,
        char character = ' ',
        int  level = 0)
        : what(what),
        depth(depth),
        character(character),
        level(level)
    {}

    explicit Indentation(int what)
        : Indentation(NodeTypes(what))
    {}

    Indentation(bool indent)
        : Indentation(indent ? Element : Nothing)
    {}

    void indent(std::ostream& out, NodeType type = Element) const
    {
        if ( type & what )
        {
            out << '\n' << std::string(level*depth, character);
        }
        else if ( type == Attribute )
        {
            out << ' ';
        }
    }

    Indentation next() const
    {
        return Indentation{what, depth, character, level+1};
    }

    bool indents_attributes() const
    {
        return what & Attribute;
    }

private:
    NodeTypes what;
    int  depth;
    char character;
    int  level;
};

class Node
{
public:
    template<class... Args>
    Node(Args&&... args)
    {
        _children.reserve(sizeof...(args));
        append(std::forward<Args>(args)...);
    }
    Node(const Node&) = default;
    Node(Node&&) = default;

    const std::vector<std::shared_ptr<Node>>& children() const
    {
        return _children;
    }

    virtual void print(std::ostream& out, const Indentation& indent) const
    {
        for ( const auto& child : _children )
            child->print(out, indent);
    }

    virtual bool is_attribute() const
    {
        return false;
    }

    virtual bool is_element() const
    {
        return false;
    }

protected:
    template<class NodeT>
        NodeT& append(std::shared_ptr<NodeT> child)
        {
            _children.push_back(child);
            return *child;
        }

    template<class NodeT>
        NodeT& append(NodeT&& child)
        {
            return append(
                std::make_shared<std::remove_cv_t<std::remove_reference_t<NodeT>>>(
                    std::forward<NodeT>(child)
            ));
        }

    template<class Head, class... Tail>
        void append(Head&& head, Tail&&... tail)
        {
            append(head);
            append(std::forward<Tail>(tail)...);
        }

    template<class ForwardIterator>
        void append_range(ForwardIterator begin, ForwardIterator end)
        {
            for ( ; begin != end; ++ begin )
                append(*begin);
        }

    template<class Sequence>
        void append_range(Sequence sequence)
        {
            _children.reserve(sequence.size());
            append_range(sequence.begin(), sequence.end());
        }

    void append(){}

private:
    std::vector<std::shared_ptr<Node>> _children;
};

inline std::ostream& operator<<(std::ostream& stream, const Node& node)
{
    node.print(stream, Indentation{});
    return stream;
}

class BlockElement : public Node
{
public:
    template<class... Args>
        BlockElement(std::string tag_name, Args&&... args)
            : Node(std::forward<Args>(args)...),
              _tag_name(std::move(tag_name))
        {}

    std::string tag_name() const
    {
        return _tag_name;
    }

    void print(std::ostream& out, const Indentation& indent) const override
    {
        indent.indent(out);
        out << '<' << tag_name();
        bool has_attribute = false;
        for ( const auto& child : children() )
        {
            if ( child->is_attribute() )
            {
                child->print(out, indent.next());
                has_attribute = true;
            }
        }
        if ( has_attribute && indent.indents_attributes() )
            indent.indent(out);
        out << '>';

        bool has_element = false;
        for ( const auto& child : children() )
        {
            if ( !child->is_attribute() )
            {
                child->print(out, indent.next());
                if ( child->is_element() )
                    has_element = true;
            }
        }
        if ( has_element )
            indent.indent(out);

        out << "</" << tag_name() << '>';
    }

    bool is_element() const override
    {
        return true;
    }

    using Node::append;

private:
    std::string _tag_name;
};

class Element : public BlockElement
{
public:
    using BlockElement::BlockElement;


    void print(std::ostream& out, const Indentation& indent) const override
    {
        for ( const auto& child : children() )
            if ( !child->is_attribute() )
                return BlockElement::print(out, indent);

        indent.indent(out);
        out << '<' << tag_name();
        for ( const auto& child : children() )
            if ( child->is_attribute() )
                child->print(out, indent.next());
        out << "/>";
    }
};

class Attribute : public Node
{
public:
    Attribute(std::string name, std::string value)
        : _name(std::move(name)),
          _value(std::move(value))
    {}

    std::string name() const
    {
        return _name;
    }

    std::string value() const
    {
        return _value;
    }

    void set_value(const std::string& value)
    {
        _value = value;
    }

    void print(std::ostream& out, const Indentation& indent) const override
    {
        indent.indent(out, Indentation::Attribute);
        out << _name << "=\"" << amp_escape(_value) << '\"';
    }

    bool is_attribute() const override
    {
        return true;
    }

private:
    std::string _name;
    std::string _value;
};

class Attributes : public Node
{
public:
    Attributes(const std::initializer_list<Attribute>& attributes)
    {
        append_range(attributes);
    }

    void print(std::ostream& out, const Indentation& indent) const override
    {
        if ( children().empty() )
            return;
        auto iter = children().begin();
        (**iter).print(out, indent);
        for ( ++iter; iter != children().end(); ++iter )
                (**iter).print(out, indent);
    }

    bool is_attribute() const override
    {
        return true;
    }
};

class Text : public Node
{
public:
    Text(std::string contents)
        : _contents(std::move(contents))
    {}

    std::string contents() const
    {
        return _contents;
    }

    void set_contents(const std::string& text)
    {
        _contents = text;
    }

    void print(std::ostream& out, const Indentation& indent) const override
    {
        out << amp_escape(_contents);
    }

private:
    std::string _contents;
};

class XmlDeclaration : public Node
{
public:
    XmlDeclaration(std::string version="1.0", std::string encoding="utf-8")
        : _version(std::move(version)),
          _encoding(std::move(encoding))
    {}

    std::string version() const
    {
        return _version;
    }

    std::string encoding() const
    {
        return _encoding;
    }

    void print(std::ostream& out, const Indentation& indent) const override
    {
        out << "<?xml version='" << _version << '\'';
        if ( !_encoding.empty() )
            out << " encoding='" << _encoding << '\'';
        out << "?>";
    }

private:
    std::string _version;
    std::string _encoding;
};

class DocType : public Node
{
public:
    DocType(std::string string)
        : _string(std::move(string))
    {}

    std::string string() const
    {
        return _string;
    }

    void print(std::ostream& out, const Indentation& indent) const override
    {
        out << "<!DOCTYPE " << _string << ">";
    }

private:
    std::string _string;
};

class Comment : public Node
{
public:
    Comment(std::string contents)
        : _contents(std::move(contents))
    {}

    std::string contents() const
    {
        return _contents;
    }

    void print(std::ostream& out, const Indentation& indent) const override
    {
        /// \todo Remove/escape --
        indent.indent(out, Indentation::Comment);
        out << "<!--";
        indent.next().indent(out, Indentation::CommentText);
        out << _contents;
        indent.indent(out, Indentation::CommentText);
        out<< "-->";
    }

private:
    std::string _contents;
};

namespace html{

class HtmlDocument : public Node
{
public:
    HtmlDocument(std::string title, BlockElement body = BlockElement{"body"})
    {
        auto shared_title = std::make_shared<Text>(std::move(title));
        _title = shared_title.get();
        auto shared_head = std::make_shared<BlockElement>("head", BlockElement{"title", shared_title});
        _head = shared_head.get();
        auto shared_body = std::make_shared<BlockElement>(std::move(body));
        _body = shared_body.get();
        append(DocType{"html"}, BlockElement{"html", shared_head, shared_body});
    }

    std::string title() const
    {
        return _title->contents();
    }

    void set_title(const std::string& title)
    {
        _title->set_contents(title);
    }

    BlockElement& head()
    {
        return *_head;
    }

    BlockElement& body()
    {
        return *_body;
    }

private:
    Text* _title;
    BlockElement* _head;
    BlockElement* _body;
};

class List : public BlockElement
{
public:
    explicit List(bool ordered = false)
        : BlockElement(ordered ? "ol" : "ul")
    {}

    template<class ElemT>
    ElemT& add_item(const std::shared_ptr<ElemT>& shared)
    {
        append(BlockElement{"li", shared});
        return *shared;
    }

    template<class ElemT>
    std::enable_if_t<std::is_base_of<Node, std::decay_t<ElemT>>::value, std::decay_t<ElemT>&>
    add_item(ElemT&& element)
    {
        auto shared = std::make_shared<std::decay_t<ElemT>>(std::forward<ElemT>(element));
        return add_item<std::decay_t<ElemT>>(shared);
    }
};

class Link : public BlockElement
{
public:
    template<class NodeT>
        Link(
            std::enable_if_t<std::is_base_of<Node, NodeT>::value, std::string> target,
            const NodeT& contents)
        : BlockElement("a")
    {
        href = &append(Attribute{"href", std::move(target)});
        append(contents);
    }

    Link(std::string target, std::string text)
        : Link(std::move(target), Text{std::move(text)})
    {}

    std::string target() const
    {
        return href->value();
    }

    void set_target(const std::string& target)
    {
        href->set_value(target);
    }

public:
    Attribute* href;
};

class Input : public Element
{
public:
    template<class... Args>
    Input(const std::string& name, std::string type, std::string value = {}, Args&&... args)
        : Element(
            "input",
            Attribute{"type", std::move(type)},
            std::forward<Args>(args)...
        )
    {
        if ( !name.empty() )
        {
            append(Attribute{"name", name});
            append(Attribute{"id", name});
        }
        _value = &append(Attribute{"value", std::move(value)});
    }

    std::string value() const
    {
        return _value->value();
    }

    void set_value(const std::string& value)
    {
        _value->set_value(value);
    }

public:
    Attribute* _value;
};

class Select : public Element
{
public:
    template<class... Args>
    Select(const std::string& name, Args&&... args)
        : Element(
            "select",
            std::forward<Args>(args)...
        )
    {
        if ( !name.empty() )
        {
            append(Attribute{"name", name});
            append(Attribute{"id", name});
        }
    }
};

class Option : public Element
{
public:
    template<class... Args>
    Option(const std::string& value, bool selected = false,
           bool disabled = false, Args&&... args)
        : Element("option", std::forward<Args>(args)...)
    {
        if ( selected )
            append(Attribute{"selected", "selected"});

        if ( disabled )
            append(Attribute{"disabled", "disabled"});

        if ( sizeof...(Args) == 0 )
            append(Text(value));
        else
            append(Attribute{"value", value});
    }
};

class TextArea : public Element
{
public:
    template<class... Args>
    TextArea(const std::string& name, std::size_t cols = 80, std::size_t rows = 24, Args&&... args)
        : Element(
            "textarea",
            Attribute{"cols", std::to_string(cols)},
            Attribute{"rows", std::to_string(rows)},
            std::forward<Args>(args)...
        )
    {
        if ( !name.empty() )
        {
            append(Attribute{"name", name});
            append(Attribute{"id", name});
        }
    }
};

class Label : public BlockElement
{
public:
    template<class NodeT>
        Label(
            std::enable_if_t<std::is_base_of<Node, NodeT>::value, std::string> target,
            const NodeT& contents)
        : BlockElement("label")
    {
        _target = &append(Attribute{"for", std::move(target)});
        append(contents);
    }

    Label(std::string target, std::string text)
        : Label(std::move(target), Text{std::move(text)})
    {}

    std::string target() const
    {
        return _target->value();
    }

    void set_target(const std::string& target)
    {
        _target->set_value(target);
    }

public:
    Attribute* _target;
};

class Table : public BlockElement
{
public:
    template<class... Args>
        explicit Table(Args&&... args)
            : BlockElement("table", std::forward<Args>(args)...)
        {}
    Table(Table& t) : Table((const Table&)t) {}
    Table(const Table&) = default;
    Table(Table&&) = default;

    template<class... Args>
        void add_data_row(Args&&... args)
    {
        append(Element{"tr", Element("td", std::forward<Args>(args))...});
    }

    template<class... Args>
        void add_header_row(Args&&... args)
    {
        append(Element{"tr", Element("th", std::forward<Args>(args))...});
    }
    template<class... Args>
        void add_row(Args&&... args)
    {
        append(Element{"tr", std::forward<Args>(args)...});
    }
};

} // namespace html

} // namespace quick_xml
} // namespace httpony
#endif // HTTPONY_QUICK_XML_HPP
