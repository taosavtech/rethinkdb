// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "containers/auth_key.hpp"

#include "containers/archive/stl_types.hpp"

auth_key_t::auth_key_t() { }

bool auth_key_t::assign_value(const std::string &new_key) {
    if (new_key.length() > static_cast<size_t>(max_length)) {
        return false;
    }

    key = new_key;
    return true;
}

RDB_IMPL_ME_SERIALIZABLE_1_SINCE_v1_13(auth_key_t, key);


bool timing_sensitive_equals(const auth_key_t &x, const auth_key_t &y) {
    const std::string &s = x.str();
    const std::string &t = y.str();

    if (s.size() != t.size()) {
        return false;
    }

    const size_t size = s.size();
    int all_equal = 1;
    for (size_t i = 0; i < size; ++i) {
        all_equal &= (s[i] == t[i]);
    }

    return all_equal;
}

