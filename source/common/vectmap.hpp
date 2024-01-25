#pragma once
//  ---------------------------------------------
//  A map implemented with vector
//  .Preserves the original insertion order
//  .Cache friendly, performance degrades with N
//  ---------------------------------------------
//  #include "vectmap.hpp" // MG::vectmap<>
//  ---------------------------------------------
#include <vector>
#include <optional>
//#include <initializer_list>

#include <fmt/format.h> // fmt::format


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace MG
{

/////////////////////////////////////////////////////////////////////////////
template<typename TKEY, typename TVAL> class vectmap final
{
 public:
    using item_type = std::pair<TKEY,TVAL>;
    using container_type = std::vector<item_type>;
    using iterator = container_type::iterator;
    using const_iterator = container_type::const_iterator;

 private:
    container_type m_v;

 public:
    //constexpr vectmap(std::initializer_list<item_type> lst)
    //  : m_v{lst}
    //   {}

    [[nodiscard]] constexpr auto size() const noexcept { return m_v.size(); }
    [[nodiscard]] constexpr bool is_empty() const noexcept { return m_v.empty(); }

    [[nodiscard]] constexpr bool operator==(vectmap<TKEY,TVAL> const& other) const noexcept
       {
        if( size()!=other.size() )
           {
            return false;
           }
        for( const_iterator it_other=other.begin(); it_other!=other.end(); ++it_other )
           {
            const_iterator it_mine = find(it_other->first);
            if( it_mine==end() or it_mine->second!=it_other->second )
               {
                return false;
               }
           }
        return true;
       }



    [[maybe_unused]] constexpr TVAL& insert_if_missing(TKEY&& key, TVAL&& val)
       {
        if( iterator it=find(key); it!=end() )
           {
            return it->second; // Already present, do nothing
           }
        return append(std::move(key), std::move(val));
       }

    [[maybe_unused]] constexpr TVAL& insert_or_assign(TKEY&& key, TVAL&& val)
       {
        if( iterator it=find(key); it!=end() )
           {
            return it->second = val; // Already present, overwrite
           }
        return append(std::move(key), std::move(val));
       }

    [[maybe_unused]] constexpr TVAL& insert_unique(TKEY&& key, TVAL&& val)
       {
        if( contains(key) )
           {
            throw std::runtime_error( fmt::format("key '{}' already present in vectmap", key) );
           }
        return append(std::move(key), std::move(val));
       }

    [[maybe_unused]] constexpr TVAL& append(TKEY&& key, TVAL&& val)
       {
        return m_v.insert(m_v.end(), item_type(std::move(key), std::move(val)))->second;
       }



    [[nodiscard]] constexpr const_iterator find(TKEY const& key) const noexcept
       {
        const_iterator it = m_v.begin();
        while( it!=m_v.end() )
           {
            if( key==it->first ) break;
            ++it;
           }
        return it;
       }

    [[nodiscard]] constexpr iterator find(TKEY const& key) noexcept
       {
        iterator it = m_v.begin();
        while( it!=m_v.end() )
           {
            if( key==it->first ) break;
            ++it;
           }
        return it;
       }

    // Deducing this:
    //template<class Self> [[nodiscard]] constexpr auto&& xxx(this Self&& self)
    //   {
    //    std::forward<Self>(self)
    //   }

    [[nodiscard]] constexpr bool contains(TKEY const& key) const noexcept
       {
        return find(key)!=end();
       }

    [[nodiscard]] constexpr auto value_of(TKEY const& key) const noexcept
       {
        std::optional<std::reference_wrapper<const TVAL>> val;
        if( const_iterator it=find(key); it!=end() )
           {
            val = std::cref(it->second);
           }
        return val;
       }

    [[nodiscard]] constexpr TVAL const& value_or(TKEY const& key, TVAL const& def) const noexcept
       {
        if( const_iterator it=find(key); it!=end() )
           {
            return it->second;
           }
        return def;
       }

    [[nodiscard]] constexpr TVAL const& operator[](TKEY const& key) const
       {
        const_iterator it = find(key);
        if( it==end() )
           {
            throw std::runtime_error( fmt::format("key '{}' not found in vectmap", key) );
           }
        return it->second;
       }

    [[nodiscard]] constexpr TVAL extract_or(TKEY const& key, TVAL const& def) noexcept
       {
        TVAL val{def};
        if( const_iterator it=find(key); it!=end() )
           {
            val = it->second;
            it = erase(it);
           }
        return val;
       }



    //#include <concepts>
    //template <typename T> concept iterator_predicate = requires(T t) { { t(const_iterator{}) } -> std::same_as<bool>; };
    constexpr void erase_if(auto condition)
       {
        for( const_iterator it=begin(); it!=end(); )
           {
            if( condition(it) )
               {
                it = erase(it);
               }
            else
               {
                ++it;
               }
           }
       }

    constexpr void erase(TKEY const& key)
       {
        for( const_iterator it = m_v.begin(); it!=m_v.end(); ++it )
           {
            if( key==it->first )
               {
                it = m_v.erase(it);
                break;
               }
           }
       }

    [[nodiscard]] constexpr const_iterator erase(const_iterator it)
       {
        return m_v.erase(it);
       }

    constexpr void clear() noexcept
       {
        m_v.clear();
       }


    [[nodiscard]] constexpr const_iterator begin() const noexcept { return m_v.begin(); }
    [[nodiscard]] constexpr const_iterator end() const noexcept { return m_v.end(); }
    [[nodiscard]] constexpr iterator begin() noexcept { return m_v.begin(); }
    [[nodiscard]] constexpr iterator end() noexcept { return m_v.end(); }


  #ifdef TEST_UNITS
    [[nodiscard]] constexpr std::string string() const
       {
        std::string s;
        const_iterator it = begin();
        s = fmt::format("{}={}", it->first, it->second);
        while( ++it!=end() )
           {
            s += fmt::format(",{}={}", it->first, it->second);
           }
        return s;
       }
  #endif
};


}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::





/////////////////////////////////////////////////////////////////////////////
#ifdef TEST_UNITS ///////////////////////////////////////////////////////////
static ut::suite<"MG::vectmap<>"> vectmap_tests = []
{////////////////////////////////////////////////////////////////////////////
    using ut::expect;
    using ut::that;
    using ut::throws;

    ut::test("MG::vectmap<std::string,std::string>") = []
       {
        MG::vectmap<std::string,std::string> v;
        expect( that % v.is_empty() and v.size()==0u );

        // Inserting
        v.insert_or_assign("key1","val1");
        expect( that % v.size()==1u and v.string()=="key1=val1"s );

        v.insert_if_missing("key2","old2");
        expect( that % v.size()==2u and v.string()=="key1=val1,key2=old2"s );

        v.insert_unique("key3","val3");
        expect( that % v.size()==3u and v.string()=="key1=val1,key2=old2,key3=val3"s );

        v.insert_if_missing("key2","val2");
        expect( that % v.size()==3u and v.string()=="key1=val1,key2=old2,key3=val3"s ) << "shouldn't modify already existing key\n";

        // Overwriting
        v.insert_or_assign("key2","val2");
        expect( that % v.size()==3u and v.string()=="key1=val1,key2=val2,key3=val3"s ) << "should have modified key2\n";
        expect( throws<std::runtime_error>([v] { ut::mut(v).insert_unique("key1",""); })) << "should throw runtime_error inserting already existing key\n";

        // Accessing not existing
        expect( throws<std::runtime_error>([&v] { [[maybe_unused]] auto s = v["x"]; })) << "should throw runtime_error accessing not existing key\n";
        expect( not v.value_of("x").has_value() );
        expect( that % v.value_or("x","def")=="def"s );

        // Accessing existing
        expect( that % v["key1"]=="val1"s and v["key2"]=="val2"s and v["key3"]=="val3"s );
        expect( that % v.value_of("key3")->get()=="val3"s );
        expect( that % v.value_or("key3","def")=="val3"s );

        // Erasing
        v.erase("x");
        expect( that % v.size()==3u and v.string()=="key1=val1,key2=val2,key3=val3"s ) << "erasing not existing key does nothing\n";
        v.erase("key1");
        expect( that % v.size()==2u and v.string()=="key2=val2,key3=val3"s ) << "erasing first key\n";
        v.clear();
        expect(that % v.is_empty() and v.size()==0u) << "should be empty after clear\n";
       };


    ut::test("MG::vectmap<int,int> loop erase") = []
       {
        MG::vectmap<int,int> v;
        v.insert_unique(1,1);
        v.insert_unique(2,2);
        v.insert_unique(3,3);
        v.insert_unique(4,4);
        v.insert_unique(5,5);
        expect( that % v.string()=="1=1,2=2,3=3,4=4,5=5"s );

        // Erase odd values
        v.erase_if( [](decltype(v)::const_iterator it) constexpr -> bool { return it->second % 2; } );
        expect( that % v.string()=="2=2,4=4"s );
       };

};///////////////////////////////////////////////////////////////////////////
#endif // TEST_UNITS ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
