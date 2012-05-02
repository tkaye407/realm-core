#ifndef __TDB_GROUP__
#define __TDB_GROUP__

#include "Table.hpp"
#include "alloc_slab.hpp"

namespace tightdb {

class Group: private Table::Parent {
public:
    Group();
    Group(const char* filename, bool readOnly=true);
    Group(const char* buffer, size_t len);
    ~Group();

    bool IsValid() const {return m_isValid;}

    size_t GetTableCount() const;
    const char* GetTableName(size_t table_ndx) const;
    bool HasTable(const char* name) const;

    Table& GetTable(const char* name);
    template<class T> T& GetTable(const char* name);

    // Serialization
    bool Write(const char* filepath);
    char* WriteToMem(size_t& len);
    
    bool Commit();

    // Conversion
    template<class S> void to_json(S& out);

#ifdef _DEBUG
    void Verify();
    void Print() const;
    MemStats Stats();
    void EnableMemDiagnostics(bool enable=true) {m_alloc.EnableDebug(enable);}
    void ToDot(std::ostream& out = std::cerr);
#endif //_DEBUG

protected:
    friend class GroupWriter;
    
    SlabAlloc& GetAllocator() {return m_alloc;}
    size_t get_free_space(size_t len, size_t& filesize, bool testOnly=false, bool ensureRest=false);
    Array& GetTopArray() {return m_top;}
    void ConnectFreeSpace(bool doConnect);
    
    // Recursively update all internal refs after commit
    void UpdateRefs(size_t TopRef);
    
    // Overriding method in ArrayParent
    virtual void update_child_ref(size_t subtable_ndx, size_t new_ref)
    {
        m_tables.Set(subtable_ndx, new_ref);
    }

    // Overriding method in Table::Parent
    virtual void child_destroyed(std::size_t) {} // Ignore

    // Overriding method in ArrayParent
    virtual size_t get_child_ref(size_t subtable_ndx) const
    {
        return m_tables.GetAsRef(subtable_ndx);
    }

    void Create();
    void CreateFromRef();

    Table& GetTable(size_t ndx);

    template<class S> size_t Write(S& out);

    // Member variables
    SlabAlloc m_alloc;
    Array m_top;
    Array m_tables;
    ArrayString m_tableNames;
    Array m_freePositions;
    Array m_freeLengths;
    Array m_cachedtables;
    bool m_isValid;
};



// Templates

template<class T> T& Group::GetTable(const char* name)
{
    const size_t n = m_tableNames.Find(name);
    if (n == size_t(-1)) {
        // Create new table
        T* const t = new T(m_alloc);
        t->m_top.SetParent(this, m_tables.Size());

        const size_t ref = t->m_top.GetRef();
        m_tables.Add(ref);
        m_tableNames.Add(name);
        m_cachedtables.Add(intptr_t(t));

        return *t;
    }
    else {
        // Get table from cache if exists, else create
        return static_cast<T&>(GetTable(n));
    }
}

template<class S>
size_t Group::Write(S& out)
{
    // Space for ref to top array
    out.write("\0\0\0\0\0\0\0\0", 8);

    // Recursively write all arrays
    // FIXME: 'valgrind' says this writes uninitialized bytes to the file/stream
    const uint64_t topPos = m_top.Write(out);
    const size_t byte_size = out.getpos();

    // top ref
    out.seek(0);
    out.write((const char*)&topPos, 8);

    // return bytes written
    return byte_size;
}

template<class S>
void Group::to_json(S& out)
{
    out << "{";

    for (size_t i = 0; i < m_tables.Size(); ++i) {
        const char* const name = m_tableNames.Get(i);
        Table& table = GetTable(i);

        if (i) out << ",";
        out << "\"" << name << "\"";
        out << ":";
        table.to_json(out);
    }

    out << "}";
}

}

#endif //__TDB_GROUP__
