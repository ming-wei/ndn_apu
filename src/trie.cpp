#include "trie.h"

#include <cassert>
#include <iostream>
#include <fstream>
#include <memory>
#include <set>
#include <limits>
#include <queue>
#include <cstring>

#include "common.h"

bool can_allocate(std::shared_ptr<TrieNode> p, int id, std::set<int> &occupied)
{
    if (occupied.find(id) != occupied.end()) return false;
    for (auto it: p->childs()) {
        char c = it.first;
        if (occupied.find(id + c) != occupied.end()) return false;
    }
    return true;
}

void allocate_recursive(std::shared_ptr<TrieNode> p, int &id, 
        std::set<int> &occupied)
{
    while (!can_allocate(p, id, occupied)) ++id;
    p->id(id++);
    for (auto it: p->childs()) {
        char c = it.first;
        occupied.insert(p->id() + c);
    }
    for (auto it: p->childs()) {
        allocate_recursive(it.second, id, occupied);
    }
}

void allocate_bfs(std::shared_ptr<TrieNode> p, int &id,
        std::set<int> &occupied)
{
    printf("allocate begin\n");
    std::queue<std::shared_ptr<TrieNode>> Q;
    Q.push(p);
    while (!Q.empty()) {
        p = Q.front();
        Q.pop();
        while (!can_allocate(p, id, occupied)) ++id;
        p->id(id++);
        for (auto it: p->childs()) {
            char c = it.first;
            occupied.insert(p->id() + c);
        }
        for (auto it: p->childs()) {
            Q.push(it.second);
        }
    }
    printf("allocate finish\n");
}

Trie::Trie(const std::vector<std::string> &fib_fnames)
{
    std::cerr << "Constructing trie..." << std::endl;
    m_root = std::make_shared<TrieNode>(-1);
    for (auto fib_fname: fib_fnames) {
        this->load_file(fib_fname);
    }
    std::cerr << "Constructing trie completed." << std::endl;
}

size_t _get_size(std::shared_ptr<TrieNode> p);

void Trie::allocate_id()
{
    std::set<int> occupied;
    int cur_id = 0;
    //allocate_recursive(m_root, cur_id, occupied);
    allocate_bfs(m_root, cur_id, occupied);
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

int Trie::query_port(const std::string &q) const
{
    auto p = m_root;
    int result = p->port();
    //printf("trie %d\n", p->id());
    for (auto c: q) {
        p = p->child(c);
        if (p == nullptr) break;
        //printf("trie %d\n", p->id());
        if (p->port() != -1) result = p->port();
    }
    return result;
}

Trie::Trie(const Trie &rhs)
{
    this->m_root = rhs.m_root->clone_recursive();
}

size_t _get_size(std::shared_ptr<TrieNode> p)
{
    if (!p) return 0;
    size_t res = 1;
    for (auto iter: p->childs()) {
        res += _get_size(iter.second);
    }
    return res;
}

size_t Trie::get_size()
{
    return _get_size(m_root);
}

int _get_largest_id(std::shared_ptr<TrieNode> p)
{
    int res = std::numeric_limits<int>::lowest();
    if (!p) return res;
    res = std::max(res, p->id());
    for (auto iter: p->childs()) {
        res = std::max(res, _get_largest_id(iter.second));
        res = std::max(res, p->id() + iter.first);
    }
    return res;
}

void _dump_to_stt(std::shared_ptr<TrieNode> p, std::vector<STTEntry> &stt,
        std::vector<int> &ports)
{
    if (!p) return;
    ports[p->id()] = p->port();
    for (auto iter: p->childs()) {
        int idx = p->id() + iter.first;
        assert(!stt[idx].valid);
        stt[idx].valid = true;
        stt[idx].c = iter.first;
        stt[idx].child_id = iter.second->id();
        _dump_to_stt(iter.second, stt, ports);
    }
}

void Trie::construct_stt(STT &stt)
{
    allocate_id();
    size_t stt_size = _get_largest_id(m_root) + 1;

    stt.entries.resize(stt_size);
    stt.ports.resize(stt_size);
    memset(stt.entries.data(), 0, sizeof(STTEntry) * stt_size);
    memset(stt.ports.data(), 0, sizeof(int) * stt_size);

    stt.root_id = m_root->id();
    _dump_to_stt(m_root, stt.entries, stt.ports);
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
