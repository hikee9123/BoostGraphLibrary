// (C) Copyright 2013 Louis Dionne
//
// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0 (See accompanying file
// LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

#include "cycle_test.hpp"
#include <boost/graph/hawick_circuits.hpp>


struct call_hawick_circuits {
    template <typename Graph, typename Visitor>
    void operator()(Graph const& g, Visitor const& v) const {
        boost::hawick_circuits(g, v);
    }
};

struct call_hawick_unique_circuits {
    template <typename Graph, typename Visitor>
    void operator()(Graph const& g, Visitor const& v) const {
        boost::hawick_unique_circuits(g, v);
    }
};

int main() {
    cycle_test(call_hawick_circuits());

    cycle_test(call_hawick_unique_circuits());

    return 0;
}
