#pragma once
//  ---------------------------------------------
//  Parse xml format (unicode text buffer)
//  ---------------------------------------------
//  #include "text_parser_xml.hpp" // text::xml::Parser
//  ---------------------------------------------
#include "text_parser_base.hpp" // text::ParserBase, parse::*
#include "string_map.hpp" // MG::string_map<>



//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace text { namespace xml
{

/////////////////////////////////////////////////////////////////////////////
class ParserEvent final
{
 public:
    using Attributes = MG::string_map<std::u32string, std::optional<std::u32string>>;

 private:
    std::u32string m_value;
    std::size_t m_start_byte_offset = 0;
    Attributes m_attributes;
    enum class type : char
       {
        NONE = 0
       ,COMMENT // <!-- ... -->
       ,TEXT // >...<
       ,OPENTAG // <tag attr1 attr2=val>
       ,CLOSETAG // </tag> or />
       ,PROCINST // <? ... ?>
       ,SPECIALBLOCK // <!xxx ... !>
       } m_type = type::NONE;

 public:
    constexpr void set_as_none() noexcept
       {
        m_type = type::NONE;
        m_value = {};
        m_attributes.clear();
       }

    constexpr void set_as_comment(std::u32string&& cmt) noexcept
       {
        m_type = type::COMMENT;
        m_value = std::move(cmt);
        m_attributes.clear();
       }
    constexpr void set_as_comment() noexcept
       {
        m_type = type::COMMENT;
        m_value = {};
        m_attributes.clear();
       }

    constexpr void set_as_text(std::u32string&& txt) noexcept
       {
        m_type = type::TEXT;
        m_value = std::move(txt);
        m_attributes.clear();
       }
    constexpr void set_as_text() noexcept
       {
        m_type = type::TEXT;
        m_value = {};
        m_attributes.clear();
       }

    constexpr void set_as_open_tag(std::u32string&& nam)
       {
        m_type = type::OPENTAG;
        m_value = std::move(nam);
        m_attributes.clear();
        if( m_value.empty() )
           {
            throw std::runtime_error{"Empty open tag"};
           }
       }

    template<typename T>
    constexpr void set_as_close_tag(T&& nam)
       {
        m_type = type::CLOSETAG;
        m_value = std::forward<T>(nam);
        m_attributes.clear();
        if( m_value.empty() )
           {
            throw std::runtime_error{"Empty open tag"};
           }
       }

    constexpr void set_as_proc_instr(std::u32string&& nam) noexcept
       {
        m_type = type::PROCINST;
        m_value = std::move(nam);
        m_attributes.clear();
       }

    constexpr void set_as_special_block(std::u32string&& nam) noexcept
       {
        m_type = type::SPECIALBLOCK;
        m_value = std::move(nam);
        m_attributes.clear();
       }

    [[nodiscard]] constexpr std::u32string const& value() const noexcept { return m_value; }

    constexpr void set_start_byte_offset(const std::size_t byte_offset) noexcept { m_start_byte_offset = byte_offset; }
    [[nodiscard]] constexpr std::size_t start_byte_offset() const noexcept { return m_start_byte_offset; }

    [[nodiscard]] constexpr Attributes const& attributes() const noexcept { return m_attributes; }
    [[nodiscard]] constexpr Attributes& attributes() noexcept { return m_attributes; }
    [[nodiscard]] constexpr bool has_attribute_with_value(const std::u32string_view key, const std::u32string_view val) const noexcept
       {
        const auto attrib = attributes().value_of(key);
        return attrib.has_value() and attrib->get().has_value() and attrib->get().value()==val;
       }

    [[nodiscard]] explicit constexpr operator bool() const noexcept { return m_type!=type::NONE; }
    [[nodiscard]] constexpr bool is_comment() const noexcept { return m_type==type::COMMENT; }
    [[nodiscard]] constexpr bool is_text() const noexcept { return m_type==type::TEXT; }
    [[nodiscard]] constexpr bool is_open_tag() const noexcept { return m_type==type::OPENTAG; }
    [[nodiscard]] constexpr bool is_close_tag() const noexcept { return m_type==type::CLOSETAG; }
    [[nodiscard]] constexpr bool is_proc_instr() const noexcept { return m_type==type::PROCINST; }
    [[nodiscard]] constexpr bool is_special_block() const noexcept { return m_type==type::SPECIALBLOCK; }

    [[nodiscard]] constexpr bool is_open_tag(const std::u32string_view nam) const noexcept { return m_type==type::OPENTAG and m_value==nam; }
    [[nodiscard]] constexpr bool is_close_tag(const std::u32string_view nam) const noexcept { return m_type==type::CLOSETAG and m_value==nam; }
};



/////////////////////////////////////////////////////////////////////////////
template<utxt::Enc ENC>
class Parser final : public text::ParserBase<ENC>
{         using inherited = text::ParserBase<ENC>;
 private:
    ParserEvent m_event; // Current event
    bool m_must_emit_tag_close_event = false; // To signal a deferred tag close

    class Options final
       {
        private:
            bool m_collect_comment_text = false; // Create event on comments
            bool m_collect_text_sections = false; // Collect text events content

        public:
            [[nodiscard]] constexpr bool is_collect_comment_text() const noexcept { return m_collect_comment_text; }
            constexpr void set_collect_comment_text(const bool b =true) noexcept { m_collect_comment_text = b; }

            [[nodiscard]] constexpr bool is_collect_text_sections() const noexcept { return m_collect_text_sections; }
            constexpr void set_collect_text_sections(const bool b =true) noexcept { m_collect_text_sections = b; }

       } m_Options;

 public:
    explicit constexpr Parser(const std::string_view bytes) noexcept
      : text::ParserBase<ENC>{bytes}
       {}

    [[nodiscard]] constexpr Options const& options() const noexcept { return m_Options; }
    [[nodiscard]] constexpr Options& options() noexcept { return m_Options; }

    [[nodiscard]] constexpr ParserEvent const& curr_event() const noexcept { return m_event; }
    [[nodiscard]] constexpr ParserEvent& mutable_curr_event() noexcept { return m_event; }

    [[maybe_unused]] constexpr ParserEvent const& next_event()
       {
        if( m_must_emit_tag_close_event )
           {
            m_must_emit_tag_close_event = false; // eat
            m_event.set_as_close_tag( m_event.value() );
           }
        else
           {
            try{
                inherited::skip_any_space();
                m_event.set_start_byte_offset( inherited::curr_codepoint_byte_offset() );
                if( inherited::has_codepoint() )
                   {
                    if( inherited::eat(U'<') )
                       {
                        parse_xml_markup();
                       }
                    else if( options().is_collect_text_sections() )
                       {
                        m_event.set_as_text( inherited::collect_until(ascii::is<U'<'>) ); // Trim right?
                       }
                    else
                       {
                        [[maybe_unused]] const auto text = inherited::get_bytes_until(ascii::is<U'<'>); // Trim right?
                        m_event.set_as_text();
                       }
                   }
                else
                   {// No more data!
                    m_event.set_as_none();
                   }
               }
            catch(parse::error&)
               {
                throw;
               }
            catch(std::runtime_error& e)
               {
                throw inherited::create_parse_error( std::string(e.what()) );
               }
           }

        return m_event;
       }


 private:
    //-----------------------------------------------------------------------
    constexpr void parse_xml_markup()
       {
        if( inherited::eat(U'!') )
           {
            if( inherited::eat(U"--") )
               {// A comment ex. <!-- ... -->
                if( options().is_collect_comment_text() )
                   {
                    m_event.set_as_comment( inherited::template collect_until<U'-',U'-',U'>'>() );
                   }
                else
                   {
                    [[maybe_unused]] const auto text = inherited::template get_bytes_until<U'-',U'-',U'>'>();
                    m_event.set_as_comment();
                   }
               }
            else if( inherited::eat(U'[') )
               {
                if( inherited::eat(U"CDATA["sv) )
                   {// A CDATA section <![CDATA[ ... ]]>
                    if( options().is_collect_text_sections() )
                       {
                        m_event.set_as_text( inherited::template collect_until<U']',U']',U'>'>() );
                       }
                    else
                       {
                        [[maybe_unused]] const auto text = inherited::template get_bytes_until<U']',U']',U'>'>();
                        m_event.set_as_text();
                       }
                   }
                else
                   {// A conditional section <![CONDITION[ ... ]]>
                    throw std::runtime_error{"Conditional sections not yet supported"};
                   }
               }
            else if( not inherited::has_codepoint() )
               {
                throw std::runtime_error{"Unclosed <!"};
               }
            else
               {// A special block: ex. <!DOCTYPE HTML>
                m_event.set_as_special_block( inherited::template collect_until<U'>'>() );
                //m_event.set_as_special_block( inherited::template collect_until(U"]>"sv) );
               }
           }
        else if( inherited::eat(U'?') )
           {// A processing instruction ex. <?xml version="1.0" encoding="utf-8"?>
            //m_event.set_as_proc_instr( inherited::template collect_until(U"?>") );
            [[maybe_unused]] const auto text = inherited::template get_bytes_until<U'?',U'>'>();
            m_event.set_as_proc_instr(U""s);
           }
        else if( inherited::eat(U'/') )
           {// A close tag
            m_event.set_as_close_tag( collect_tag_name() );
            inherited::skip_any_space();
            if( not inherited::eat(U'>') )
               {
                throw std::runtime_error{"Invalid close tag"};
               }
           }
        else
           {// A tag
            m_event.set_as_open_tag( collect_tag_name() );
            inherited::skip_any_space();
            if( not inherited::eat(U'>') )
               {
                // Collect attributes
                auto namval = collect_attribute();
                while( not namval.first.empty() )
                   {
                    if( m_event.attributes().contains(namval.first) )
                       {
                        throw inherited::create_parse_error( fmt::format("Duplicated attribute `{}`", utxt::to_utf8(namval.first)) );
                       }
                    m_event.attributes().append( std::move(namval) );
                    namval = collect_attribute();
                   }

                // Detect immediate tag close
                if( inherited::eat(U'/') )
                   {// Next event will be a tag close
                    m_must_emit_tag_close_event = true;
                    //skip_any_space();
                   }

                // Expect >
                if( not inherited::eat(U'>') )
                   {
                    throw inherited::create_parse_error( fmt::format("Tag `{}` must be closed with >", utxt::to_utf8(m_event.value())) );
                   }
               }
           }
       }

    //-----------------------------------------------------------------------
    [[nodiscard]] constexpr ParserEvent::Attributes::item_type collect_attribute()
       {
        assert( not inherited::got_space() ); // collect_attribute() expects non-space char
        ParserEvent::Attributes::item_type namval;

        namval.first = collect_attr_name();
        if( not namval.first.empty() )
           {// Check possible value
            inherited::skip_any_space();
            if( inherited::eat(U'=') )
               {
                inherited::skip_any_space();
                namval.second = inherited::eat(U'\"') ? collect_quoted_attr_value()
                                                      : collect_unquoted_attr_value();
                inherited::skip_any_space();
               }
           }
        return namval;
       }

    //-----------------------------------------------------------------------
    [[nodiscard]] constexpr std::u32string collect_tag_name()
       {
        inherited::skip_any_space();
        try{
            return inherited::collect_until(ascii::is_space_or_any_of<U'>',U'/'>, ascii::is_punct_and_none_of<U'-',U':'>);
           }
        catch(std::exception& e)
           {
            throw inherited::create_parse_error( fmt::format("Invalid tag name: {}"sv, e.what()) );
           }
       }

    //-----------------------------------------------------------------------
    [[nodiscard]] constexpr std::u32string collect_attr_name()
       {
        assert( not inherited::got_space() ); // collect_unquoted_attr_name() expects non-space char"
        try{
            return inherited::collect_until(ascii::is_space_or_any_of<U'=',U'>',U'/'>, ascii::is_punct_and_none_of<U'-'>);
           }
        catch(std::exception& e)
           {
            throw inherited::create_parse_error( fmt::format("Invalid attribute name: {}"sv, e.what()) );
           }
       }

    //-----------------------------------------------------------------------
    [[nodiscard]] constexpr std::u32string collect_quoted_attr_value()
       {
        try{
            return inherited::collect_until_and_skip(ascii::is<U'\"'>, ascii::is_endline<char32_t>);
           }
        catch(std::exception& e)
           {
            throw inherited::create_parse_error( fmt::format("Invalid attribute quoted value: {}"sv, e.what()) );
           }
       }

    //-----------------------------------------------------------------------
    [[nodiscard]] constexpr std::u32string collect_unquoted_attr_value()
       {
        assert( not inherited::got_space() ); // collect_unquoted_attr_value() expects non-space char"
        try{
            return inherited::collect_until(ascii::is_space_or_any_of<U'>',U'/'>, ascii::is_any_of<U'<',U'=',U'\"'>);
           }
        catch(std::exception& e)
           {
            throw inherited::create_parse_error( fmt::format("Invalid attribute value: {}"sv, e.what()) );
           }
       }
};


}}//:::::::::::::::::::::::::::::: text::xml ::::::::::::::::::::::::::::::::




/////////////////////////////////////////////////////////////////////////////
#ifdef TEST_UNITS ///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//---------------------------------------------------------------------------
[[nodiscard]] constexpr std::string to_string( text::xml::ParserEvent::Attributes const& attrs )
   {
    std::string s;
    auto it = attrs.begin();
    if( it!=attrs.end() )
       {
        s += utxt::to_utf8(it->first);
        if( it->second.has_value() )
           {
            s += '=';
            s += utxt::to_utf8(it->second.value());
           }

        while( ++it!=attrs.end() )
           {
            s += ',';
            s += utxt::to_utf8(it->first);
            if( it->second.has_value() )
               {
                s += '=';
                s += utxt::to_utf8(it->second.value());
               }
           }
       }
    return s;
   }
//---------------------------------------------------------------------------
[[nodiscard]] constexpr std::string to_string(text::xml::ParserEvent const& ev)
   {
    if( ev.is_open_tag() )
        return fmt::format("open tag: {} ({})", utxt::to_utf8(ev.value()), to_string(ev.attributes()));

    else if( ev.is_close_tag() )
        return fmt::format("close tag: {}", utxt::to_utf8(ev.value()));

    else if( ev.is_comment() )
        return fmt::format("comment: {}", utxt::to_utf8(ev.value()));

    else if( ev.is_text() )
        return fmt::format("text: {}", utxt::to_utf8(ev.value()));

    else if( ev.is_proc_instr() )
        return fmt::format("proc-instr: {}", utxt::to_utf8(ev.value()));

    else if( ev.is_special_block() )
        return fmt::format("spec-block: {}", utxt::to_utf8(ev.value()));

    return "(none)";
   }

/////////////////////////////////////////////////////////////////////////////
static ut::suite<"text::xml::Parser"> text_parser_xml_tests = []
{
using ut::expect;
using ut::that;
using ut::throws;

ut::test("ParserEvent") = []
   {
    text::xml::ParserEvent event;

    expect( that % not event ) << "empty event should evaluate false\n";

    event.set_as_comment(U"cmt"s);
    expect( that % event.is_comment() and event.value()==U"cmt"sv ) << "should be a comment event\n";

    event.set_as_text(U"txt"s);
    expect( that % event.is_text() and event.value()==U"txt"sv ) << "should be a text event\n";

    event.set_as_open_tag(U"open-tag"s);
    expect( that % event.is_open_tag(U"open-tag"sv) ) << "should be an open tag event\n";

    event.set_as_close_tag(U"close-tag"s);
    expect( that % event.is_close_tag(U"close-tag"sv) ) << "should be a close tag event\n";

    event.set_as_proc_instr(U"proc-instr"s);
    expect( that % event.is_proc_instr() and event.value()==U"proc-instr"sv ) << "should be a proc-instr event\n";

    event.set_as_special_block(U"block"s);
    expect( that % event.is_special_block() and event.value()==U"block"sv ) << "should be a special block event\n";
   };


auto notify_sink = [](const std::string_view msg) -> void { ut::log << "\033[33m" "parser: " "\033[0m" << msg; };
ut::test("generic xml") = [&notify_sink]
   {
    const std::string_view buf =
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<!DOCTYPE doctype>\n"
        //"<!DOCTYPE doctype [\n"
        //"<!ELEMENT root (child+)>\n"
        //"]>\n"
        "<!-- comment -->\n"
        "<tag1/><tag2 attr1=\"1\" attr2=2 attr3/>\n"
        "<tag3>blah</tag3>\n"
        "< nms:tag4 \n attr1=\"1&lt;2\" \n"
        " attr2=\"42\" \n "
        ">blah</ nms:tag4 >\n"
        "  some text\n"
        "<![CDATA[\n"
        "  Some <>not parsed<> text\n"
        "]]>\n"
        "<root>\n"
        "    <child key1=123 key2=\"quoted value\"/>\n"
        "    <child key1 key2=\"blah blah\">\n"
        "        &apos;text&apos;\n"
        "        <subchild>\n"
        "            text text text\n"
        "            text text text\n"
        "        </subchild>\n"
        "    </child>\n"
        "</root>\n";

    text::xml::Parser<utxt::Enc::UTF8> parser{buf};
    parser.options().set_collect_comment_text(true);
    parser.options().set_collect_text_sections(true);
    parser.set_on_notify_issue(notify_sink);
    std::size_t n_event = 0u;
    try{
        while( const text::xml::ParserEvent& event = parser.next_event() )
           {
            //ut::log << "\033[33m" << to_string(event) << "\033[36m" "(event " << n_event+1u << " line " << parser.curr_line() << ")\n" "\033[0m";
            switch( ++n_event )
               {
                case  1: expect(event.is_proc_instr()) << "got: " << to_string(event) << '\n'; break;
                case  2: expect(event.is_special_block()) << "got: " << to_string(event) << '\n'; break;
                case  3: expect(event.is_comment()) << "got: " << to_string(event) << '\n'; break;
                case  4: expect(event.is_open_tag(U"tag1") and event.attributes().size()==0) << "got: " << to_string(event) << '\n'; break;
                case  5: expect(event.is_close_tag(U"tag1")) << "got: " << to_string(event) << '\n'; break;
                case  6: expect(event.is_open_tag(U"tag2") and to_string(event.attributes())=="attr1=1,attr2=2,attr3") << "got: " << to_string(event) << '\n'; break;
                case  7: expect(event.is_close_tag(U"tag2")) << "got: " << to_string(event) << '\n'; break;
                case  8: expect(event.is_open_tag(U"tag3") and event.attributes().size()==0) << "got: " << to_string(event) << '\n'; break;
                case  9: expect(event.is_text() and event.value()==U"blah") << "got: " << to_string(event) << '\n'; break;
                case 10: expect(event.is_close_tag(U"tag3")) << "got: " << to_string(event) << '\n'; break;
                case 11: expect(event.is_open_tag(U"nms:tag4") and to_string(event.attributes())=="attr1=1&lt;2,attr2=42") << "got: " << to_string(event) << '\n'; break;
                case 12: expect(event.is_text() and event.value()==U"blah") << "got: " << to_string(event) << '\n'; break;
                case 13: expect(event.is_close_tag(U"nms:tag4")) << "got: " << to_string(event) << '\n'; break;
                case 14: expect(event.is_text() and event.value()==U"some text\n") << "got: " << to_string(event) << '\n'; break;
                case 15: expect(event.is_text() and event.value()==U"\n  Some <>not parsed<> text\n") << "got: " << to_string(event) << '\n'; break;
                case 16: expect(event.is_open_tag(U"root") and event.attributes().size()==0) << "got: " << to_string(event) << '\n'; break;
                case 17: expect(event.is_open_tag(U"child") and to_string(event.attributes())=="key1=123,key2=quoted value") << "got: " << to_string(event) << '\n'; break;
                case 18: expect(event.is_close_tag(U"child")) << "got: " << to_string(event) << '\n'; break;
                case 19: expect(event.is_open_tag(U"child") and to_string(event.attributes())=="key1,key2=blah blah") << "got: " << to_string(event) << '\n'; break;
                case 20: expect(event.is_text() and event.value()==U"&apos;text&apos;\n        ") << "got: " << to_string(event) << '\n'; break;
                case 21: expect(event.is_open_tag(U"subchild") and event.attributes().size()==0) << "got: " << to_string(event) << '\n'; break;
                case 22: expect(event.is_text()) << "got: " << to_string(event) << '\n'; break;
                case 23: expect(event.is_close_tag(U"subchild")) << "got: " << to_string(event) << '\n'; break;
                case 24: expect(event.is_close_tag(U"child")) << "got: " << to_string(event) << '\n'; break;
                case 25: expect(event.is_close_tag(U"root")) << "got: " << to_string(event) << '\n'; break;
                default: expect(false) << "unexpected event: " << to_string(event) << '\n';
               }
           }
       }
    catch( parse::error& e )
       {
        ut::log << "\033[35m" "Exception: " "\033[31m" << e.what() << "\033[0m" "(event " << n_event << " line " << e.line() << ")\n";
       }
    expect( that % n_event==25u ) << "events number should match";
   };

ut::test("unclosed comment") = [&notify_sink]
   {
    const std::string_view buf = "<!--\n\n\n\n";
    text::xml::Parser<utxt::Enc::UTF8> parser{buf};
    parser.set_on_notify_issue(notify_sink);
    expect( throws<parse::error>([&parser] { [[maybe_unused]] auto ev = parser.next_event(); }) ) << "unclosed comment should throw\n";
   };

ut::test("interface.xml sample") = [&notify_sink]
   {
    const std::string_view buf =
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<?xml-stylesheet type=\"text/xsl\" href=\"Interface2XHTML.xsl\"?>\n"
        "<!--\n"
        "    Dizionario interfaccia unificata macchine Macotec\n"
        "    ©2017-2022 gattanini@macotec.it\n"
        "-->\n"
        "<interface version=\"2022-09-06\"\n"
        "           name=\"MacoLayer\"\n"
        "           xmlns=\"http://www.macotec.it\">\n"
        "\n"
        "<!-- +------------------------------------------------------------------+\n"
        "     ¦ Statistics and maintenance                                       ¦\n"
        "     +------------------------------------------------------------------+ -->\n"
        "<group name=\"statistics\">\n"
        "\n"
        "    <res    id=\"sheets-done\" tags=\"statistics,counter,sheets,done\" access=\"r\"\n"
        "            type=\"int\">\n"
        "        <text lang=\"en\" label=\"Done sheets\">Processed sheets count</text>\n"
        "        <text lang=\"it\" label=\"Lastre lavorate\">Contatore lastre lavorate</text>\n"
        "    </res>\n"
        "\n"
        "    <res    id=\"buffer-width\" tags=\"settings,machine,modules,size,width,buffer\" access=\"r\"\n"
        "            type=\"double\" quantity=\"length\" unit=\"mm\" unit-coeff=\"0.001\"\n"
        "            range=\"0:6000\" gran=\"0.1\" default=\"2400\">\n"
        "        <text lang=\"en\" label=\"Buffer width\">Buffer longitudinal size</text>\n"
        "        <text lang=\"it\" label=\"Largh polmone\">Larghezza del polmone</text>\n"
        "    </res>\n"
        "\n"
        "</group> <!-- statistics -->\n"
        "\n"
        "</interface>\n";
    text::xml::Parser<utxt::Enc::UTF8> parser{buf};
    parser.set_on_notify_issue(notify_sink);
    std::size_t n_event = 0u;
    try{
        while( const text::xml::ParserEvent& event = parser.next_event() )
           {
            //ut::log << "\033[33m" << to_string(event) << "\033[36m" "(event " << n_event+1u << " line " << parser.curr_line() << ")\n" "\033[0m";
            switch( ++n_event )
               {
                case  1: expect(event.is_proc_instr()) << "got: " << to_string(event) << '\n'; break;
                case  2: expect(event.is_proc_instr()) << "got: " << to_string(event) << '\n'; break;
                case  3: expect(event.is_comment()) << "got: " << to_string(event) << '\n'; break;
                case  4: expect(event.is_open_tag(U"interface") and event.attributes().size()==3) << "got: " << to_string(event) << '\n'; break;
                case  5: expect(event.is_comment()) << "got: " << to_string(event) << '\n'; break;
                case  6: expect(event.is_open_tag(U"group") and event.attributes()[U"name"]==U"statistics") << "got: " << to_string(event) << '\n'; break;
                case  7: expect(event.is_open_tag(U"res") and event.attributes()[U"id"]==U"sheets-done") << "got: " << to_string(event) << '\n'; break;
                case  8: expect(event.is_open_tag(U"text") and event.attributes()[U"lang"]==U"en") << "got: " << to_string(event) << '\n'; break;
                case  9: expect(event.is_text()) << "got: " << to_string(event) << '\n'; break;
                case 10: expect(event.is_close_tag(U"text")) << "got: " << to_string(event) << '\n'; break;
                case 11: expect(event.is_open_tag(U"text") and event.attributes()[U"lang"]==U"it") << "got: " << to_string(event) << '\n'; break;
                case 12: expect(event.is_text()) << "got: " << to_string(event) << '\n'; break;
                case 13: expect(event.is_close_tag(U"text")) << "got: " << to_string(event) << '\n'; break;
                case 14: expect(event.is_close_tag(U"res")) << "got: " << to_string(event) << '\n'; break;
                case 15: expect(event.is_open_tag(U"res") and event.attributes()[U"id"]==U"buffer-width") << "got: " << to_string(event) << '\n'; break;
                case 16: expect(event.is_open_tag(U"text") and event.attributes()[U"lang"]==U"en") << "got: " << to_string(event) << '\n'; break;
                case 17: expect(event.is_text()) << "got: " << to_string(event) << '\n'; break;
                case 18: expect(event.is_close_tag(U"text")) << "got: " << to_string(event) << '\n'; break;
                case 19: expect(event.is_open_tag(U"text") and event.attributes()[U"lang"]==U"it") << "got: " << to_string(event) << '\n'; break;
                case 20: expect(event.is_text()) << "got: " << to_string(event) << '\n'; break;
                case 21: expect(event.is_close_tag(U"text")) << "got: " << to_string(event) << '\n'; break;
                case 22: expect(event.is_close_tag(U"res")) << "got: " << to_string(event) << '\n'; break;
                case 23: expect(event.is_close_tag(U"group")) << "got: " << to_string(event) << '\n'; break;
                case 24: expect(event.is_comment()) << "got: " << to_string(event) << '\n'; break;
                case 25: expect(event.is_close_tag(U"interface")) << "got: " << to_string(event) << '\n'; break;
                default: expect(false) << "unexpected event:\"" << to_string(event) << "\"\n";
                
               }
           }
       }
    catch( parse::error& e )
       {
        ut::log << "\033[35m" "Exception: " "\033[31m" << e.what() << "\033[0m" "(event " << n_event << " line " << e.line() << ")\n";
       }
    expect( that % n_event==25u ) << "events number should match";
   };

};///////////////////////////////////////////////////////////////////////////
#endif // TEST_UNITS ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
