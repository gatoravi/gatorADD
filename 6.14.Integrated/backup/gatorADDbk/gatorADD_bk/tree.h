/*
 * Copyright (C) 2009 Andre Wehe
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

// bidirectional tree

#ifndef TREE_H
#define TREE_H

#include "common.h"
#include "util.h"
#include <vector>
#include <stack>
#include <iostream>
#include <boost/foreach.hpp>
#include <boost/dynamic_bitset.hpp>
#include <math.h>

namespace aw {

#include <iterator>

enum traversal_states {PREORDER, INORDER, POSTORDER, NOTRAVERSAL};
static const unsigned int NONODE = UINT_MAX;

template<class VALUE>
class TreeTemplate {
    // some data type definitions
    public: typedef VALUE value_type;
    protected: typedef TreeTemplate<value_type> this_type;

    // list of edges/nodes adjacent to a node
    // iteratable by std::forward_iterator
    protected: class AdjacentList {
        protected: std::vector<unsigned int> data; // store the adjacent nodes
        public: inline unsigned int size() {
            return data.size();
        }
        // insert an adjacent node
        public: inline void insert(const unsigned int v) {
            data.push_back(v);
        }
        // removes an adjacent node
        // return true is node was found (and remove)
        public: inline bool remove(const unsigned int v) {
            for (unsigned int i=0,iEE=data.size(); i<iEE; ++i){
                if (data[i] == v) {
                    data[i] = data[iEE-1];
                    data.resize(iEE-1);
                    return true;
                }
            }
            return false;
        }
        // true if node is adjacent
        public: inline bool exist(const unsigned int v) {
            for (std::vector<unsigned int>::iterator itr=data.begin(), itrEE=data.end(); itr!=itrEE; itr++) {
                if (*itr == v) return true;
            }
            return false;
        }
        // std::forward_iterator for AdjacentList container
        public: class Iterator : public std::iterator<std::forward_iterator_tag, unsigned int> {
            public: Iterator(std::vector<unsigned int>::iterator itr_) : itr(itr_) {}
            public: inline bool operator==(const Iterator &r) {
                return itr == r.itr;
            }
            public: inline bool operator!=(const Iterator &r) {
                return itr != r.itr;
            }
            public: inline Iterator& operator++() {
                ++itr;
                return *this;
            }
            public: inline Iterator operator++(int) {
                Iterator tmp(*this);
                ++(*this);
                return tmp;
            }
            public: inline unsigned int& operator*() {
                return *itr;
            }
            public: inline unsigned int* operator->() {
                return &*(AdjacentList::Iterator)*this;
            }
            protected: std::vector<unsigned int>::iterator itr;
        };
        // default iterator is Iterator (std::forward_iterator)
        public: typedef Iterator iterator;
        public: typedef Iterator const_iterator;
        public: inline Iterator begin() { return Iterator(data.begin()); }
        public: inline Iterator end() { return Iterator(data.end()); }
    };

    // list of child nodes
    // iteratable by std::forward_iterator
    protected: class ChildrenList {
        public: ChildrenList(AdjacentList *adj_, unsigned int parent_) : adj(adj_), parent(parent_) { }
        protected: AdjacentList *adj;
        protected: unsigned int parent;
        public: inline unsigned int size() {
            return (adj->exist(parent)) ? adj->size() -1 : adj->size();
        }
        // insert a child node
        public: inline void insert(const unsigned int v) {
            adj->insert(v);
        }
        // removes a child node
        // return true is node was found (and remove)
        public: inline bool remove(const unsigned int v) {
            return adj->remove(v);
        }
        // true if node is a child
        public: inline bool exist(const unsigned int v) {
            if (v == parent) return false;
            return adj->exist(v);
        }
        // std::forward_iterator for ChildrenList container
        public: class Iterator : public std::iterator<std::forward_iterator_tag, unsigned int> {
            public: Iterator(typename AdjacentList::iterator itr_, unsigned int parent_) : itr(itr_), parent(parent_) { }
            public: inline bool operator==(const Iterator &r) {
                if (itr == r.itr) return true;
                if (*itr == parent) {
                    ++itr;
                    return itr == r.itr;
                }
                return false;
            }
            public: inline bool operator!=(const Iterator &r) {
                return itr != r.itr;
            }
            public: inline Iterator& operator++() {
                ++itr;
                return *this;
            }
            public: inline Iterator operator++(int) {
                Iterator tmp(*this);
                ++(*this);
                return tmp;
            }
            public: inline unsigned int& operator*() {
                if (*itr == parent) ++itr;
                return *itr;
            }
            public: inline unsigned int* operator->() {
                return &*(ChildrenList::Iterator)*this;
            }
            protected: typename AdjacentList::iterator itr;
            protected: unsigned int parent;
        };
        // default iterator is Iterator (std::forward_iterator)
        public: typedef Iterator iterator;
        public: typedef Iterator const_iterator;
        public: inline Iterator begin() const { return Iterator(adj->begin(),parent); }
        public: inline Iterator end() const { return Iterator(adj->end(),parent); }
    };

    // store node related data
    protected: class Node {
        public: value_type value;
        public: AdjacentList adjacent_nodes;
    };
    protected: typedef Node node_type;

    // store all nodes where the index is their ID
    protected: std::vector<node_type> nodes23;

    // reserve memory for nodes being added in the future
    public: void node_reserve(const unsigned int s) {
        nodes23.reserve(s);
    }

    // access the node data - for internal use only
    protected: inline node_type& node(const unsigned int v) { return nodes23[v]; }

    // access node
    public: inline value_type& value(const unsigned int v) { return nodes23[v].value; }

    // define a special value for unrooted trees
    public: unsigned int root;

    // keeps track of number of edges
    protected: unsigned int edge_count;

    // default constructor
    public: TreeTemplate() {
        clear();
    }

    public: void clear() {
        root = NONODE;
        edge_count = 0;
        nodes23.clear();
    }

    // return the id of a node
    protected: inline unsigned int idx(node_type &n) { return &n - &nodes23[0]; }

    // swap the content of 2 trees
    public: void swap(this_type &r) {
        r.nodes23.swap(nodes23);
        util::swap(root, r.root);
        util::swap(edge_count, r.edge_count);
    }

    // true if the tree contains no nodes (and no edges)
    public: inline bool empty() { return nodes23.empty(); }

    // adjacent nodes in form of an iteratable container
    public: inline AdjacentList& adjacent(const unsigned int v) { return node(v).adjacent_nodes; }

    // adjacent nodes in a vector
    public: inline void adjacent(const unsigned int v, std::vector<unsigned int> &vec) {
        AdjacentList &adj = adjacent(v);
        vec.resize(adj.size());
        unsigned int i = 0;
        BOOST_FOREACH(const unsigned int &u, adj) {
            vec[i] = u;
            ++i;
        }
    }
    public: inline std::vector<unsigned int> adjacent_vector(const unsigned int v) {
        std::vector<unsigned int> vec;
        adjacent(v,vec);
        return vec;
    }

    // adjacent nodes in an array (no out of bounds checking, make sure array has enough space)
    public: inline unsigned int adjacent(const unsigned int v, unsigned int * const arr) {
        unsigned int *arr2 = arr;
        BOOST_FOREACH(const unsigned int &u, adjacent(v)) {
            *arr2 = u;
            ++arr2;
        }
        return arr2 - arr;
    }

    // child nodes in form of an iteratable container
    public: inline ChildrenList children(const unsigned int v, const unsigned int parent) {
        AdjacentList * const adj = &adjacent(v);
        return ChildrenList(adj,parent);
    }

    // child nodes in a vector (explicit defined parent required)
    public: inline void children(const unsigned int v, const unsigned int parent, std::vector<unsigned int> &vec) {
        vec.reserve(degree(v)-1);
        BOOST_FOREACH(const unsigned int &u, adjacent(v)) {
            if (u != parent) {
                vec.push_back(u);
            }
        }
    }
    public: inline std::vector<unsigned int> children_vector(const unsigned int v, const unsigned int parent) {
        std::vector<unsigned int> vec;
        children(v, parent, vec);
        return vec;
    }

    // child nodes in an array (no out of bounds checking, make sure array has enough space)
    public: inline unsigned int children(const unsigned int v, const unsigned int parent, unsigned int * const arr) {
        unsigned int *arr2 = arr;
        BOOST_FOREACH(const unsigned int &u, adjacent(v)) {
            if (u != parent) {
                *arr2 = u;
                ++arr2;
            }
        }
        return arr2 - arr;
    }

    // degree of a node
    public: inline unsigned int degree(const unsigned int v) {
        return adjacent(v).size();
    }

    // true if the node is a leaf (including single nodes)
    public: inline bool is_leaf(const unsigned int v) { return (degree(v) <= 1); }

    // connect 2 nodes with an edge
    public: inline void add_edge(const unsigned int v, const unsigned int u) {
        adjacent(v).insert(u);
        adjacent(u).insert(v);
        ++edge_count;
    }

    // remove an edge between 2 nodes
    // return false edge does not exist
    public: inline bool remove_edge(const unsigned int v, const unsigned int u) {
        if (!adjacent(v).remove(u)) return false;
        if (!adjacent(u).remove(v)) return false;
        --edge_count;
        return true;
    }

<<<<<<< HEAD
    // number of edges in the tree
=======
    // number of edges int the tree
>>>>>>> c00678f46a203f307fa8d0280b4b7d38c47027e7
    public: inline unsigned int edge_size() { return edge_count; }
    // number of nodes in the tree
    public: inline unsigned int node_size() { return nodes23.size(); }

    // create a new node and add it to the tree. the node is disconnected from the tree
    public: inline unsigned int new_node() {
        unsigned int l = nodes23.size();
        nodes23.resize(l+1);
        return l;
    }

//     // delete a node
//     // note: results in the change of the id of the most recent added node
//     public: inline bool del_node(const unsigned int i) { return del_node(node(i)); }
//     protected: inline bool del_node(node_type &v) {
//         if (!v.adjacent23.empty()) WARNING_return("delete failed - node is still connected");
//         unsigned int last = nodes23.size()-1;
//         if (idx(v) != last) {
//             v = node(last);
//         }
//         nodes23.resize(last);
//         return true;
//     }

    // remove all edges from a node
    // note: the node is not deleted
    public: inline void disconnect_node(const unsigned int v) {
        while (degree(v) != 0) remove_edge(v,*adjacent(v).begin());
    }

    // true if the root is defined
    public: inline bool is_rooted() {
        return root != NONODE;
    }

    // true if the root is NOT defined
    public: inline bool is_unrooted() {
        return root == NONODE;
    }

    // undefine the root
    public: inline void unroot() {
        root = NONODE;
    }

    // true if all nodes form one connected componet
    public: inline bool is_connected() {
        boost::dynamic_bitset<> visited(nodes23.size());
        visited[0] = 1;
        BOOST_FOREACH(node_type &n, nodes23) {
            unsigned int i = idx(n);
            BOOST_FOREACH(const unsigned int &j, adjacent(n)) {
                if (j>i) visited[j] = 1;
            }
        }
        for (boost::dynamic_bitset<>::size_type i = 0; i < visited.size(); ++i) {
            if (visited[i] == false) return false;
        }
        return true;
    }

    // number of connected components
    public: unsigned int components() {
        boost::dynamic_bitset<> visited(nodes23.size());
        visited[0] = 1;
        BOOST_FOREACH(node_type &n, nodes23) {
            unsigned int i = idx(n);
            BOOST_FOREACH(const unsigned int &j, adjacent(n)) {
                if (j>i) visited[j] = 1;
            }
        }
        unsigned int comp = 1;
        for (boost::dynamic_bitset<>::size_type i = 0; i < visited.size(); ++i) {
            if (visited[i] == false) comp++;
        }
        return comp;
    }

    // contract the edge (v,u); node u becomes disconnected
    public: inline void contract_edge(node_type &v, node_type &u) { contract_edge(v,u,idx(v),idx(u)); }
    public: inline void contract_edge(const unsigned int v, const unsigned int u) { contract_edge(node(v),node(u),v,u); }
    protected: inline void contract_edge(node_type &v, node_type &u, const unsigned int vi, const unsigned int ui) {
        remove_edge(v,u);
        std::vector<unsigned int> adj; adjacent(u,adj);
        BOOST_FOREACH(const unsigned int &i, adj) {
            remove_edge(i,ui);
            add_edge(i,vi);
        }
    }

    // contract a chain-node (node of degree 2) (excluding root node)
    public: inline bool contract_chain_node(const unsigned int v) {
        if (degree(v) != 2) return false;
        if (v == root) return false;

        unsigned int l[2],i = 0;
        BOOST_FOREACH(unsigned int j, adjacent(v)) {
            l[i] = j;
            i++;
        }
        remove_edge(l[0],v);
        remove_edge(l[1],v);
        add_edge(l[0],l[1]);
        return true;
    }

    // contract a chain of nodes (connected nodes of degree 2)
    public: inline bool contract_chain(const unsigned int v) {
        std::stack<unsigned int> adj; adj.push(v);
        while (!adj.empty()) {
            const unsigned int u = adj.top(); adj.pop();
            BOOST_FOREACH(const unsigned int &i, adjacent(u)) {
                if (degree(i) == 2) adj.push(i);
            }
            if (!contract_chain_node(u)) return false;
        }
        return true;
    }

    // contract all chain-nodes (node of degree 2) in the tree
    public: inline bool contract_all_chains() {
        for (unsigned int i=0,iEE=nodes23.size();i<iEE;++i) {
            if (!contract_chain_node(i)) return false;
        }
        return true;
    }

    // cut out a leaf node
    public: inline bool trim_leaf(const unsigned int v) {
        if (degree(v) != 1) return false;
        const unsigned int p = *adjacent(v).begin();
        remove_edge(v,p);
        contract_chain(p);
        return true;
    }

    // cut out a set of leaves
    public: template<class T> inline bool trim_leaves(T leaves) {
        BOOST_FOREACH(const unsigned int &v, leaves) if (!trim_leaf(v)) return false;
        return true;
    }

    // cut out a set of leaves (rooted)
    public: template<class T> inline bool trim_leaves_rooted(T leaves) {
        const bool ret = trim_leaves(leaves);
        trim_root();
        return ret;
    }

    // cut down the root if it is has only one child (rooted)
    public: inline bool trim_root() {
        if (!is_rooted()) return false;
        if (degree(root) != 1) return false;
        while (degree(root) == 1) {
            const unsigned int c = *adjacent(root).begin();
            remove_edge(c,root);
            root = c;
        }
        return true;
    }

    // SPR move to the root
    // u defines the subtree root node, and pu the parent node of u
    public: inline bool spr_to_root(const unsigned int u, const unsigned int pu) {
        if (!is_rooted()) return false;
        if (u == root) return false; // someone wants me to move the entire tree -> can't do that
        if (pu == root) return true;
        // prune subtree with parent
        std::vector<unsigned int> ch; children(pu,u,ch);
        BOOST_FOREACH(const unsigned int &i, ch) {
            if (!remove_edge(pu,i)) return false;
        }
        if (ch.size() == 2) {
            add_edge(ch[0],ch[1]);
        } else
        if (ch.size() > 2) {
            const unsigned int n = new_node();
            BOOST_FOREACH(const unsigned int &i, ch) {
                add_edge(n,i);
            }
        }
        // regraft subtree to root
        add_edge(pu,root);
        // define the new root
        root = pu;
        return true;
    }

    // SPR move from the root
    // c is the root of the prune subtree (must be a child of the root)
    // r will be the new root (must be a sibling of c)
    // (u,v) is the edge c will be regrafted into
    public: inline bool spr_from_root(const unsigned int c, const unsigned int r, const unsigned int u, const unsigned int v) {
        if (!is_rooted()) return false;
        if ((root == u) || (root == v)) {
            return true;
        }
        // prune subtree with parent
        BOOST_FOREACH(const unsigned int &i, children(root,c)) {
            if (!remove_edge(root,i)) return false;
        }
        // regraft subtree into (u,v)
        if (!remove_edge(u,v)) return false;
        add_edge(root,u);
        add_edge(root,v);
        // define the new root
        root = r;
        return true;
    }

    // SPR move of subtree rooted at n with parent pn into edge (u,v) such that {u,pn,v} becomes a path
    public: inline bool spr(const unsigned int n, const unsigned int pn, const unsigned int u, const unsigned int v) {
        if (pn == u) return true;
        if (pn == v) return true;
        // prune subtree with parent
        std::vector<unsigned int> ch; children(pn,n,ch);
        BOOST_FOREACH(const unsigned int &i, ch) {
            if (!remove_edge(pn,i)) return false;
        }
        if (ch.size() == 2) {
            add_edge(ch[0],ch[1]);
        } else
        if (ch.size() > 2) {
            const unsigned int n = new_node();
            BOOST_FOREACH(const unsigned int &i, ch) {
                add_edge(n,i);
            }
        }
        // regraft subtree into (u,v)
        if (!remove_edge(u,v)) return false;
        add_edge(pn,u);
        add_edge(pn,v);
        return true;
    }

    // reroot a subtree rooted at n with parent pn into edge (u,v) such that {u,n,v} becomes a path
    public: inline bool reroot(const unsigned int n, const unsigned int pn, const unsigned int u, const unsigned int v) {
        if (n == u) return true;
        if (n == v) return true;
        // cut current rooting
        std::vector<unsigned int> ch; children(n,pn,ch);
        BOOST_FOREACH(const unsigned int &i, ch) {
            if (!remove_edge(n,i)) return false;
        }
        if (ch.size() == 2) {
            add_edge(ch[0],ch[1]);
        } else
        if (ch.size() > 2) {
            const unsigned int n = new_node();
            BOOST_FOREACH(const unsigned int &i, ch) {
                add_edge(n,i);
            }
        }
        // connect root into (u,v)
        if (!remove_edge(u,v)) return false;
        add_edge(n,u);
        add_edge(n,v);
        return true;
    }

    // iterator stuff
    // -----------------------------------------------------------------------------------
    // std::forward_iterator for iterating through all nodes
    public: class Iterator_allnodes : public std::iterator<std::forward_iterator_tag, unsigned int> {
        public: Iterator_allnodes(unsigned int idx_) : idx(idx_) { }
        public: inline bool operator==(const Iterator_allnodes &r) { return idx == r.idx; }
        public: inline bool operator!=(const Iterator_allnodes &r) { return idx != r.idx; }
        public: inline Iterator_allnodes& operator++() {
            ++idx;
            return *this;
        }
        public: inline Iterator_allnodes operator++(int) {
            Iterator_allnodes tmp(*this);
            ++(*this);
            return tmp;
        }
        public: inline unsigned int& operator*() { return idx; }
        public: inline unsigned int* operator->() { return &*(Iterator_allnodes)*this; }
        protected: unsigned int idx;
    };
    // default iterator is Iterator_allnodes (std::forward_iterator)
    public: typedef Iterator_allnodes iterator;
    public: typedef Iterator_allnodes const_iterator;
    public: inline Iterator_allnodes begin() { return Iterator_allnodes(0); }
    public: inline Iterator_allnodes end() { return Iterator_allnodes(node_size()); }

    // -----------------------------------------------------------------------------------
    // std::forward_iterator for iterating through all leaf nodes
    public: class Iterator_leafnodes : public Iterator_allnodes {
        public: Iterator_leafnodes(unsigned int idx_, this_type *ptr_) : Iterator_allnodes(idx_), ptr(ptr_) {
            if ((idx != ptr->node_size()) && !ptr->is_leaf(idx)) ++(*this);
        }
        public: inline Iterator_leafnodes& operator++() {
            while (++idx < ptr->node_size()) if (ptr->is_leaf(idx)) break;
            return *this;
        }
        public: inline Iterator_leafnodes operator++(int) {
            Iterator_leafnodes tmp(*this);
            ++(*this);
            return tmp;
        }
        protected: this_type *ptr;
        public: using Iterator_allnodes::idx;
    };
    public: typedef Iterator_leafnodes iterator_leafnodes;
    public: typedef Iterator_leafnodes const_iterator_leafnodes;
    public: inline Iterator_leafnodes begin_leafnodes() { return Iterator_leafnodes(0,this); }
    public: inline Iterator_leafnodes end_leafnodes() { return Iterator_leafnodes(node_size(),this); }

    // -----------------------------------------------------------------------------------
    // std::forward_iterator for iterating through all internal nodes
    public: class Iterator_internalnodes : public Iterator_allnodes {
        public: Iterator_internalnodes(unsigned int idx_, this_type *ptr_) : Iterator_allnodes(idx_), ptr(ptr_) {
            if ((idx != ptr->node_size()) && ptr->is_leaf(idx)) ++(*this);
        }
        public: inline Iterator_internalnodes& operator++() {
            while (++idx < ptr->node_size()) if (!ptr->is_leaf(idx)) break;
            return *this;
        }
        public: inline Iterator_internalnodes operator++(int) {
            Iterator_internalnodes tmp(*this);
            ++(*this);
            return tmp;
        }
        protected: this_type *ptr;
        public: using Iterator_allnodes::idx;
    };
    public: typedef Iterator_internalnodes iterator_internalnodes;
    public: typedef Iterator_internalnodes const_iterator_internalnodes;
    public: inline Iterator_internalnodes begin_internalnodes() { return Iterator_internalnodes(0,this); }
    public: inline Iterator_internalnodes end_internalnodes() { return Iterator_internalnodes(node_size(),this); }

    // -----------------------------------------------------------------------------------
    // std::forward_iterator for iterating through all nodes according to DFS
    public: class Iterator_dfs : public std::iterator<std::forward_iterator_tag, unsigned int> {
        public: Iterator_dfs(unsigned int idx_, this_type *ptr_) : ptr(ptr_), idx(idx_) {
            if (idx != ptr->node_size()) dfs_init();
        }
        public: Iterator_dfs(unsigned int idx_, this_type *ptr_, const unsigned int v, const unsigned int p = NONODE) : ptr(ptr_), idx(idx_) {
            if (idx != ptr->node_size()) dfs_init(v,p);
        }
        public: inline bool operator==(const Iterator_dfs &r) { return idx == r.idx; }
        public: inline bool operator!=(const Iterator_dfs &r) { return idx != r.idx; }
        public: inline Iterator_dfs& operator++() {
            dfs_next();
            if (dfs_end()) idx = ptr->node_size();
            return *this;
        }
        public: inline Iterator_dfs operator++(int) {
            Iterator_dfs tmp(*this);
            ++(*this);
            return tmp;
        }
        public: inline unsigned int& operator*() { return idx; }
        public: inline unsigned int* operator->() { return &*(Iterator_dfs)*this; }
        protected: this_type *ptr;
        // DFS public
        public: unsigned int idx;
        public: unsigned int lvl;
        public: unsigned int parent;
        public: traversal_states direction;
        // DFS interna
        protected: typedef util::triplet<unsigned int,typename ChildrenList::iterator,typename ChildrenList::iterator> dfs_item;
        protected: std::vector<dfs_item> dfs_stack;
        protected: inline void dfs_init(const unsigned int v) {
            this_type &tree = *ptr;
            dfs_stack.reserve(log(tree.node_size())/3/*3 = (10*log(2))*/);
            // fill the data structure with the start node
            idx = v;
            lvl = 0;
            parent = NONODE;
            direction = PREORDER;
            ChildrenList ch = tree.children(v,NONODE); // parent will be ignored in children
            dfs_stack.push(dfs_item(idx,ch.begin(),ch.end()));
        }
        protected: inline void dfs_init(const unsigned int v, const unsigned int p) {
            this_type &tree = *ptr;
            dfs_stack.reserve(log(tree.node_size())/3/*3 = (10*log(2))*/);
            // fill the data structure with the start node
            idx = v;
            lvl = 0;
            parent = p;
            direction = PREORDER;
            ChildrenList ch = tree.children(v,p); // parent will be ignored in children
            dfs_stack.push_back(dfs_item(idx,ch.begin(),ch.end()));
        }
        protected: inline void dfs_init() {
            this_type &tree = *ptr;
            unsigned int root;
            if (tree.root == NONODE) {
                WARNING("traversal without root node");
                root = 0;
            } else root = tree.root;
            // fill the data structure with the root node
            if (tree.empty()) {
                idx = UINT_MAX;
                lvl = 0;
                parent = UINT_MAX;
                direction = NOTRAVERSAL;
            } else {
                idx = root;
                lvl = 0;
                parent = NONODE;
                direction = PREORDER;
            }
            ChildrenList adj = tree.children(idx,parent); // parent will be ignored in children
            dfs_stack.push_back(dfs_item(idx,adj.begin(),adj.end()));
        }
        public: inline void skip() {
            dfs_item &w = *dfs_stack.rbegin();
            w.second = w.third;
        }
        protected: inline unsigned int get_parent() {
            if (dfs_stack.size() < 2) return NONODE;
            return (++(dfs_stack.rbegin()))->first;
        }
        protected: inline void dfs_next() {
            if (dfs_end()) return;
            switch (direction) {
                case PREORDER: { // last traversal step was preorder
                    dfs_item &w = *dfs_stack.rbegin();
                    if (w.second == w.third) { // leaf
                        direction = INORDER;
                    } else { // not leaf
                        parent = idx;
                        idx = *(w.second);
                        ++lvl;
                        this_type &tree = *ptr;
                        ChildrenList ch = tree.children(idx,parent);
                        dfs_stack.push_back(dfs_item(idx,ch.begin(),ch.end()));
                    }
                } break;
                case INORDER: { // last traversal step was inorder
                    dfs_item &w = *dfs_stack.rbegin();
                    if (w.second == w.third) { // leaf
                        direction = POSTORDER;
                    } else { // not leaf
                        direction = PREORDER;
                        parent = idx;
                        idx = *(w.second);
                        ++lvl;
                        this_type &tree = *ptr;
                        ChildrenList ch = tree.children(idx,parent);
                        dfs_stack.push_back(dfs_item(idx,ch.begin(),ch.end()));
                    }
                } break;
                case POSTORDER: { // last traversal step was postorder
                    dfs_stack.pop_back();
                    if (dfs_end()) { // end of DFS
                        idx = UINT_MAX;
                        lvl = 0;
                        parent = UINT_MAX;
                        direction = NOTRAVERSAL;
                    } else { // more nodes in DFS
                        dfs_item &w = *dfs_stack.rbegin();
                        idx = w.first;
                        --lvl;
                        parent = get_parent();
                        ++w.second;
                        direction = w.second == w.third ? POSTORDER : INORDER;
                    }
                } break;
                default: ERROR_exit("broken traversal");
            }
        }
        protected: inline bool dfs_end() { return dfs_stack.empty(); }
    };
    public: typedef Iterator_dfs iterator_dfs;
    public: typedef Iterator_dfs const_iterator_dfs;
    public: inline Iterator_dfs begin_dfs() { return Iterator_dfs(0,this); }
    public: inline Iterator_dfs begin_dfs(const unsigned int v) { return Iterator_dfs(0,this,v); }
    public: inline Iterator_dfs begin_dfs(const unsigned int v, const unsigned int p) { return Iterator_dfs(0,this,v,p); }
    public: inline Iterator_dfs end_dfs() { return Iterator_dfs(node_size(),this); }

    // -----------------------------------------------------------------------------------
    // std::forward_iterator for iterating through all nodes according to EULERTOUR
    public: class Iterator_eulertour : public Iterator_dfs {
        public: Iterator_eulertour(unsigned int idx_, this_type *ptr_) : Iterator_dfs(idx_,ptr_) {
            if ((idx != ptr->node_size()) && !is_candidate()) ++(*this);
        }
        public: Iterator_eulertour(unsigned int idx_, this_type *ptr_, const unsigned int v, const unsigned int p = NONODE) : Iterator_dfs(idx_,ptr_,v,p) {
            if ((idx != ptr->node_size()) && !is_candidate()) ++(*this);
        }
        private: inline bool is_candidate() {
            return ((!ptr->is_leaf(idx)) || (direction == INORDER));
        }
        public: inline Iterator_eulertour& operator++() {
            for (;;) {
                dfs_next();
                if (dfs_end()) {
                    idx = ptr->node_size();
                    break;
                }
                if (is_candidate()) break;
            }
            return *this;
        }
        public: inline Iterator_eulertour operator++(int) {
            Iterator_eulertour tmp(*this);
            ++(*this);
            return tmp;
        }
        protected: using Iterator_dfs::ptr;
        public: using Iterator_dfs::idx;
        public: using Iterator_dfs::lvl;
        public: using Iterator_dfs::parent;
        public: using Iterator_dfs::direction;
        protected: using Iterator_dfs::dfs_next;
        protected: using Iterator_dfs::dfs_end;
    };
    public: typedef Iterator_eulertour iterator_eulertour;
    public: typedef Iterator_eulertour const_iterator_eulertour;
    public: inline Iterator_eulertour begin_eulertour() { return Iterator_eulertour(0,this); }
    public: inline Iterator_eulertour begin_eulertour(const unsigned int v) { return Iterator_eulertour(0,this,v); }
    public: inline Iterator_eulertour begin_eulertour(const unsigned int v, const unsigned int p) { return Iterator_eulertour(0,this,v,p); }
    public: inline Iterator_eulertour end_eulertour() { return Iterator_eulertour(node_size(),this); }

    // -----------------------------------------------------------------------------------
    // std::forward_iterator for iterating through all nodes according to PREORDER
    public: class Iterator_preorder : public Iterator_dfs {
        public: Iterator_preorder(unsigned int idx_, this_type *ptr_) : Iterator_dfs(idx_,ptr_) {
            if ((idx != ptr->node_size()) && !is_candidate()) ++(*this);
        }
        public: Iterator_preorder(unsigned int idx_, this_type *ptr_, const unsigned int v, const unsigned int p = NONODE) : Iterator_dfs(idx_,ptr_,v,p) {
            if ((idx != ptr->node_size()) && !is_candidate()) ++(*this);
        }
        private: inline bool is_candidate() {
            return (direction == PREORDER);
        }
        public: inline Iterator_preorder& operator++() {
            for (;;) {
                dfs_next();
                if (dfs_end()) {
                    idx = ptr->node_size();
                    break;
                }
                if (is_candidate()) break;
            }
            return *this;
        }
        public: inline Iterator_preorder operator++(int) {
            Iterator_preorder tmp(*this);
            ++(*this);
            return tmp;
        }
        protected: using Iterator_dfs::ptr;
        public: using Iterator_dfs::idx;
        public: using Iterator_dfs::lvl;
        public: using Iterator_dfs::parent;
        public: using Iterator_dfs::direction;
        protected: using Iterator_dfs::dfs_next;
        protected: using Iterator_dfs::dfs_end;
    };
    public: typedef Iterator_preorder iterator_preorder;
    public: typedef Iterator_preorder const_iterator_preorder;
    public: inline Iterator_preorder begin_preorder() { return Iterator_preorder(0,this); }
    public: inline Iterator_preorder begin_preorder(const unsigned int v) { return Iterator_preorder(0,this,v); }
    public: inline Iterator_preorder begin_preorder(const unsigned int v, const unsigned int p) { return Iterator_preorder(0,this,v,p); }
    public: inline Iterator_preorder end_preorder() { return Iterator_preorder(node_size(),this); }

    // -----------------------------------------------------------------------------------
    // std::forward_iterator for iterating through all nodes according to INORDER
    public: class Iterator_inorder : public Iterator_dfs {
        public: Iterator_inorder(unsigned int idx_, this_type *ptr_) : Iterator_dfs(idx_,ptr_) {
            if ((idx != ptr->node_size()) && !is_candidate()) ++(*this);
        }
        public: Iterator_inorder(unsigned int idx_, this_type *ptr_, const unsigned int v, const unsigned int p = NONODE) : Iterator_dfs(idx_,ptr_,v,p) {
            if ((idx != ptr->node_size()) && !is_candidate()) ++(*this);
        }
        private: inline bool is_candidate() {
            return (direction == INORDER);
        }
        public: inline Iterator_inorder& operator++() {
            for (;;) {
                dfs_next();
                if (dfs_end()) {
                    idx = ptr->node_size();
                    break;
                }
                if (is_candidate()) break;
            }
            return *this;
        }
        public: inline Iterator_inorder operator++(int) {
            Iterator_inorder tmp(*this);
            ++(*this);
            return tmp;
        }
        protected: using Iterator_dfs::ptr;
        public: using Iterator_dfs::idx;
        public: using Iterator_dfs::lvl;
        public: using Iterator_dfs::parent;
        public: using Iterator_dfs::direction;
        protected: using Iterator_dfs::dfs_next;
        protected: using Iterator_dfs::dfs_end;
    };
    public: typedef Iterator_inorder iterator_inorder;
    public: typedef Iterator_inorder const_iterator_inorder;
    public: inline Iterator_inorder begin_inorder() { return Iterator_inorder(0,this); }
    public: inline Iterator_inorder begin_inorder(const unsigned int v) { return Iterator_inorder(0,this,v); }
    public: inline Iterator_inorder begin_inorder(const unsigned int v, const unsigned int p) { return Iterator_inorder(0,this,v,p); }
    public: inline Iterator_inorder end_inorder() { return Iterator_inorder(node_size(),this); }

    // -----------------------------------------------------------------------------------
    // std::forward_iterator for iterating through all nodes according to POSTORDER
    public: class Iterator_postorder : public Iterator_dfs {
        public: Iterator_postorder(unsigned int idx_, this_type *ptr_) : Iterator_dfs(idx_,ptr_) {
            if ((idx != ptr->node_size()) && !is_candidate()) ++(*this);
        }
        public: Iterator_postorder(unsigned int idx_, this_type *ptr_, const unsigned int v, const unsigned int p = NONODE) : Iterator_dfs(idx_,ptr_,v,p) {
            if ((idx != ptr->node_size()) && !is_candidate()) ++(*this);
        }
        private: inline bool is_candidate() {
            return (direction == POSTORDER);
        }
        public: inline Iterator_postorder& operator++() {
            for (;;) {
                dfs_next();
                if (dfs_end()) {
                    idx = ptr->node_size();
                    break;
                }
                if (is_candidate()) break;
            }
            return *this;
        }
        public: inline Iterator_postorder operator++(int) {
            Iterator_postorder tmp(*this);
            ++(*this);
            return tmp;
        }
        protected: using Iterator_dfs::ptr;
        public: using Iterator_dfs::idx;
        public: using Iterator_dfs::lvl;
        public: using Iterator_dfs::parent;
        public: using Iterator_dfs::direction;
        protected: using Iterator_dfs::dfs_next;
        protected: using Iterator_dfs::dfs_end;
    };
    public: typedef Iterator_postorder iterator_postorder;
    public: typedef Iterator_postorder const_iterator_postorder;
    public: inline Iterator_postorder begin_postorder() { return Iterator_postorder(0,this); }
    public: inline Iterator_postorder begin_postorder(const unsigned int v) { return Iterator_postorder(0,this,v); }
    public: inline Iterator_postorder begin_postorder(const unsigned int v, const unsigned int p) { return Iterator_postorder(0,this,v,p); }
    public: inline Iterator_postorder end_postorder() { return Iterator_postorder(node_size(),this); }
};

typedef TreeTemplate<util::empty> Tree;

} // end of namespace

#endif
