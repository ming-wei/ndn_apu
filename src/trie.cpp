#include "trie.h"

#include <cassert>
#include <iostream>
#include <fstream>
#include <memory>

#include "common.h"

Trie::Trie(const std::vector<std::string> &fib_fnames)
{
    std::cerr << "Constructing trie..." << std::endl;
    m_root = std::make_shared<TrieNode>(-1);
    for (auto fib_fname: fib_fnames) {
        this->load_file(fib_fname);
    }
    std::cerr << "Constructing trie completed." << std::endl;
}

void Trie::load_file(const std::string &fib_fname)
{
    std::cerr << "Loading " << fib_fname << "..." << std::endl;
    std::ifstream is(fib_fname);
    assert(is);
    while (is) {
        std::string line, path, port_str;

        std::getline(is, line);
        if (line.empty()) {
            continue;
        }
        size_t comma_pos = line.find_last_of(",");
        if (comma_pos == std::string::npos) {
            std::cerr << "no comma found in `" << line << "'" << std::endl;
            continue;
        }
        assert(comma_pos != std::string::npos);
        path = line.substr(0, comma_pos);
        port_str = line.substr(comma_pos + 1);
        int port = std::stoi(port_str, nullptr);

        this->insert_entry(path, port);
    }
}

void Trie::insert_entry(const std::string &path, int port)
{
    auto p = m_root;
    for (char ch: path) {
        p = p->child(ch, true);
    }
    p->port(port);
}

int Trie::query_port(const std::string &q)
{
    auto p = m_root;
    int result = -1;
    for (auto c: q) {
        p = p->child(c);
        if (p == nullptr) break;
        if (p->port() != -1) result = p->port();
    }
    return result;
}

Trie::Trie(const Trie &rhs)
{
    this->m_root = rhs.m_root->clone_recursive();
}

/// Trie Node

TrieNode::TrieNode(int port): m_port(port)
{
}

std::shared_ptr<TrieNode> TrieNode::child(char c, bool create_if_not_exist) 
{
    auto iter = m_childs.lower_bound(c);
    if (iter == m_childs.end() || iter->first != c) {
        // node doesn't exist
        if (create_if_not_exist) {
            iter = m_childs.insert(iter, std::make_pair(c, 
                        std::make_shared<TrieNode>()));
            return iter->second;
        } else {
            return nullptr;
        }
    }
    return iter->second;
}

std::shared_ptr<TrieNode> TrieNode::clone_recursive() const
{
    auto res = std::make_shared<TrieNode>(this->m_port);
    for (auto iter: this->childs()) {
        char ch = iter.first;
        std::shared_ptr<TrieNode> child = iter.second;
        res->m_childs[ch] = child->clone_recursive();
    }
    return res;
}

// vim: ft=cpp.doxygen
