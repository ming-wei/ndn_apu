#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>

#include "common.h"

class TrieNode;

class Trie {
    public:
        explicit Trie(const std::vector<std::string> &fib_fnames);
        explicit Trie(const Trie &rhs);
        ~Trie() = default;

        void load_file(const std::string &fib_fname);
        void insert_entry(const std::string &path, int port);
        int query_port(const std::string &q) const;

        void construct_stt(STT &stt);

        void aggregate_and_export(const std::string &fib_fname);
        size_t get_size();
        std::shared_ptr<TrieNode> root() const
        { return m_root; }

    private:
        std::shared_ptr<TrieNode> m_root;
        void allocate_id();
};

class TrieNode {
    public:
        explicit TrieNode(int port = -1);

        std::shared_ptr<TrieNode> child(char c, 
                bool create_if_not_exist = false);
        const std::map<char, std::shared_ptr<TrieNode>> &childs() const {
            return m_childs;
        }
        std::shared_ptr<TrieNode> clone_recursive() const;
        TrieNode &port(int port) {
            m_port = port;
            return *this;
        }
        int port() const {
            return m_port;
        }
        TrieNode &id(int id) {
            m_id = id;
            return *this;
        }
        int id() const {
            return m_id;
        }
    private:
        std::map<char, std::shared_ptr<TrieNode>> m_childs;
        int m_port;
        int m_id;
};
