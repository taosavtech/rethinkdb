// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rdb_protocol/term.hpp"

#include "arch/address.hpp"
#include "clustering/administration/jobs/report.hpp"
#include "containers/cow_ptr.hpp"
#include "concurrency/cross_thread_watchable.hpp"
#include "rdb_protocol/counted_term.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/func.hpp"
#include "rdb_protocol/minidriver.hpp"
#include "rdb_protocol/stream_cache.hpp"
#include "rdb_protocol/term_walker.hpp"
#include "rdb_protocol/validate.hpp"
#include "rdb_protocol/terms/terms.hpp"
#include "thread_local.hpp"

namespace ql {

counted_t<const term_t> compile_term(compile_env_t *env, protob_t<const Term> t) {
    // HACK: per @srh, use unlimited array size at compile time
    ql::configured_limits_t limits = ql::configured_limits_t::unlimited;
    switch (t->type()) {
    case Term::DATUM:              return make_datum_term(t, limits);
    case Term::MAKE_ARRAY:         return make_make_array_term(env, t);
    case Term::MAKE_OBJ:           return make_make_obj_term(env, t);
    case Term::BINARY:             return make_binary_term(env, t);
    case Term::VAR:                return make_var_term(env, t);
    case Term::JAVASCRIPT:         return make_javascript_term(env, t);
    case Term::HTTP:               return make_http_term(env, t);
    case Term::ERROR:              return make_error_term(env, t);
    case Term::IMPLICIT_VAR:       return make_implicit_var_term(env, t);
    case Term::RANDOM:             return make_random_term(env, t);
    case Term::DB:                 return make_db_term(env, t);
    case Term::TABLE:              return make_table_term(env, t);
    case Term::GET:                return make_get_term(env, t);
    case Term::GET_ALL:            return make_get_all_term(env, t);
    case Term::EQ:                 // fallthru
    case Term::NE:                 // fallthru
    case Term::LT:                 // fallthru
    case Term::LE:                 // fallthru
    case Term::GT:                 // fallthru
    case Term::GE:                 return make_predicate_term(env, t);
    case Term::NOT:                return make_not_term(env, t);
    case Term::ADD:                // fallthru
    case Term::SUB:                // fallthru
    case Term::MUL:                // fallthru
    case Term::DIV:                return make_arith_term(env, t);
    case Term::MOD:                return make_mod_term(env, t);
    case Term::CONTAINS:           return make_contains_term(env, t);
    case Term::APPEND:             return make_append_term(env, t);
    case Term::PREPEND:            return make_prepend_term(env, t);
    case Term::DIFFERENCE:         return make_difference_term(env, t);
    case Term::SET_INSERT:         return make_set_insert_term(env, t);
    case Term::SET_INTERSECTION:   return make_set_intersection_term(env, t);
    case Term::SET_UNION:          return make_set_union_term(env, t);
    case Term::SET_DIFFERENCE:     return make_set_difference_term(env, t);
    case Term::SLICE:              return make_slice_term(env, t);
    case Term::GET_FIELD:          return make_get_field_term(env, t);
    case Term::INDEXES_OF:         return make_indexes_of_term(env, t);
    case Term::KEYS:               return make_keys_term(env, t);
    case Term::OBJECT:             return make_object_term(env, t);
    case Term::HAS_FIELDS:         return make_has_fields_term(env, t);
    case Term::WITH_FIELDS:        return make_with_fields_term(env, t);
    case Term::PLUCK:              return make_pluck_term(env, t);
    case Term::WITHOUT:            return make_without_term(env, t);
    case Term::MERGE:              return make_merge_term(env, t);
    case Term::LITERAL:            return make_literal_term(env, t);
    case Term::ARGS:               return make_args_term(env, t);
    case Term::BETWEEN:            return make_between_term(env, t);
    case Term::CHANGES:            return make_changes_term(env, t);
    case Term::REDUCE:             return make_reduce_term(env, t);
    case Term::MAP:                return make_map_term(env, t);
    case Term::FILTER:             return make_filter_term(env, t);
    case Term::CONCAT_MAP:          return make_concatmap_term(env, t);
    case Term::GROUP:              return make_group_term(env, t);
    case Term::ORDER_BY:            return make_orderby_term(env, t);
    case Term::DISTINCT:           return make_distinct_term(env, t);
    case Term::COUNT:              return make_count_term(env, t);
    case Term::SUM:                return make_sum_term(env, t);
    case Term::AVG:                return make_avg_term(env, t);
    case Term::MIN:                return make_min_term(env, t);
    case Term::MAX:                return make_max_term(env, t);
    case Term::UNION:              return make_union_term(env, t);
    case Term::NTH:                return make_nth_term(env, t);
    case Term::BRACKET:            return make_bracket_term(env, t);
    case Term::LIMIT:              return make_limit_term(env, t);
    case Term::SKIP:               return make_skip_term(env, t);
    case Term::INNER_JOIN:         return make_inner_join_term(env, t);
    case Term::OUTER_JOIN:         return make_outer_join_term(env, t);
    case Term::EQ_JOIN:            return make_eq_join_term(env, t);
    case Term::ZIP:                return make_zip_term(env, t);
    case Term::RANGE:              return make_range_term(env, t);
    case Term::INSERT_AT:          return make_insert_at_term(env, t);
    case Term::DELETE_AT:          return make_delete_at_term(env, t);
    case Term::CHANGE_AT:          return make_change_at_term(env, t);
    case Term::SPLICE_AT:          return make_splice_at_term(env, t);
    case Term::COERCE_TO:          return make_coerce_term(env, t);
    case Term::UNGROUP:            return make_ungroup_term(env, t);
    case Term::TYPE_OF:             return make_typeof_term(env, t);
    case Term::UPDATE:             return make_update_term(env, t);
    case Term::DELETE:             return make_delete_term(env, t);
    case Term::REPLACE:            return make_replace_term(env, t);
    case Term::INSERT:             return make_insert_term(env, t);
    case Term::DB_CREATE:          return make_db_create_term(env, t);
    case Term::DB_DROP:            return make_db_drop_term(env, t);
    case Term::DB_LIST:            return make_db_list_term(env, t);
    case Term::DB_CONFIG:          return make_db_config_term(env, t);
    case Term::TABLE_CREATE:       return make_table_create_term(env, t);
    case Term::TABLE_DROP:         return make_table_drop_term(env, t);
    case Term::TABLE_LIST:         return make_table_list_term(env, t);
    case Term::TABLE_CONFIG:       return make_table_config_term(env, t);
    case Term::TABLE_STATUS:       return make_table_status_term(env, t);
    case Term::TABLE_WAIT:         return make_table_wait_term(env, t);
    case Term::RECONFIGURE:        return make_reconfigure_term(env, t);
    case Term::REBALANCE:          return make_rebalance_term(env, t);
    case Term::SYNC:               return make_sync_term(env, t);
    case Term::INDEX_CREATE:       return make_sindex_create_term(env, t);
    case Term::INDEX_DROP:         return make_sindex_drop_term(env, t);
    case Term::INDEX_LIST:         return make_sindex_list_term(env, t);
    case Term::INDEX_STATUS:       return make_sindex_status_term(env, t);
    case Term::INDEX_WAIT:         return make_sindex_wait_term(env, t);
    case Term::INDEX_RENAME:       return make_sindex_rename_term(env, t);
    case Term::FUNCALL:            return make_funcall_term(env, t);
    case Term::BRANCH:             return make_branch_term(env, t);
    case Term::ANY:                return make_any_term(env, t);
    case Term::ALL:                return make_all_term(env, t);
    case Term::FOR_EACH:            return make_foreach_term(env, t);
    case Term::FUNC:               return make_counted<func_term_t>(env, t);
    case Term::ASC:                return make_asc_term(env, t);
    case Term::DESC:               return make_desc_term(env, t);
    case Term::INFO:               return make_info_term(env, t);
    case Term::MATCH:              return make_match_term(env, t);
    case Term::SPLIT:              return make_split_term(env, t);
    case Term::UPCASE:             return make_upcase_term(env, t);
    case Term::DOWNCASE:           return make_downcase_term(env, t);
    case Term::SAMPLE:             return make_sample_term(env, t);
    case Term::IS_EMPTY:           return make_is_empty_term(env, t);
    case Term::DEFAULT:            return make_default_term(env, t);
    case Term::JSON:               return make_json_term(env, t);
    case Term::TO_JSON_STRING:     return make_to_json_string_term(env, t);
    case Term::ISO8601:            return make_iso8601_term(env, t);
    case Term::TO_ISO8601:         return make_to_iso8601_term(env, t);
    case Term::EPOCH_TIME:         return make_epoch_time_term(env, t);
    case Term::TO_EPOCH_TIME:      return make_to_epoch_time_term(env, t);
    case Term::NOW:                return make_now_term(env, t);
    case Term::IN_TIMEZONE:        return make_in_timezone_term(env, t);
    case Term::DURING:             return make_during_term(env, t);
    case Term::DATE:               return make_date_term(env, t);
    case Term::TIME_OF_DAY:        return make_time_of_day_term(env, t);
    case Term::TIMEZONE:           return make_timezone_term(env, t);
    case Term::TIME:               return make_time_term(env, t);

    case Term::YEAR:               return make_portion_term(env, t, pseudo::YEAR);
    case Term::MONTH:              return make_portion_term(env, t, pseudo::MONTH);
    case Term::DAY:                return make_portion_term(env, t, pseudo::DAY);
    case Term::DAY_OF_WEEK:        return make_portion_term(env, t,
                                                            pseudo::DAY_OF_WEEK);
    case Term::DAY_OF_YEAR:        return make_portion_term(env, t,
                                                            pseudo::DAY_OF_YEAR);
    case Term::HOURS:              return make_portion_term(env, t, pseudo::HOURS);
    case Term::MINUTES:            return make_portion_term(env, t, pseudo::MINUTES);
    case Term::SECONDS:            return make_portion_term(env, t, pseudo::SECONDS);

    case Term::MONDAY:             return make_constant_term(env, t, 1, "monday");
    case Term::TUESDAY:            return make_constant_term(env, t, 2, "tuesday");
    case Term::WEDNESDAY:          return make_constant_term(env, t, 3, "wednesday");
    case Term::THURSDAY:           return make_constant_term(env, t, 4, "thursday");
    case Term::FRIDAY:             return make_constant_term(env, t, 5, "friday");
    case Term::SATURDAY:           return make_constant_term(env, t, 6, "saturday");
    case Term::SUNDAY:             return make_constant_term(env, t, 7, "sunday");
    case Term::JANUARY:            return make_constant_term(env, t, 1, "january");
    case Term::FEBRUARY:           return make_constant_term(env, t, 2, "february");
    case Term::MARCH:              return make_constant_term(env, t, 3, "march");
    case Term::APRIL:              return make_constant_term(env, t, 4, "april");
    case Term::MAY:                return make_constant_term(env, t, 5, "may");
    case Term::JUNE:               return make_constant_term(env, t, 6, "june");
    case Term::JULY:               return make_constant_term(env, t, 7, "july");
    case Term::AUGUST:             return make_constant_term(env, t, 8, "august");
    case Term::SEPTEMBER:          return make_constant_term(env, t, 9, "september");
    case Term::OCTOBER:            return make_constant_term(env, t, 10, "october");
    case Term::NOVEMBER:           return make_constant_term(env, t, 11, "november");
    case Term::DECEMBER:           return make_constant_term(env, t, 12, "december");
    case Term::GEOJSON:            return make_geojson_term(env, t);
    case Term::TO_GEOJSON:         return make_to_geojson_term(env, t);
    case Term::POINT:              return make_point_term(env, t);
    case Term::LINE:               return make_line_term(env, t);
    case Term::POLYGON:            return make_polygon_term(env, t);
    case Term::DISTANCE:           return make_distance_term(env, t);
    case Term::INTERSECTS:         return make_intersects_term(env, t);
    case Term::INCLUDES:           return make_includes_term(env, t);
    case Term::CIRCLE:             return make_circle_term(env, t);
    case Term::GET_INTERSECTING:   return make_get_intersecting_term(env, t);
    case Term::FILL:               return make_fill_term(env, t);
    case Term::GET_NEAREST:        return make_get_nearest_term(env, t);
    case Term::UUID:               return make_uuid_term(env, t);
    case Term::POLYGON_SUB:        return make_polygon_sub_term(env, t);
    default: unreachable();
    }
    unreachable();
}

void run(protob_t<Query> q,
         rdb_context_t *ctx,
         signal_t *interruptor,
         stream_cache_t *stream_cache,
         ip_and_port_t const &peer,
         Response *res) {
    try {
        validate_pb(*q);
    } catch (const base_exc_t &e) {
        fill_error(res, Response::CLIENT_ERROR, e.what(), backtrace_t());
        return;
    }
#ifdef INSTRUMENT
    debugf("Query: %s\n", q->DebugString().c_str());
#endif // INSTRUMENT

    cond_t job_interruptor;
    map_insertion_sentry_t<uuid_u, query_job_t> job_sentry(
        ctx->get_query_jobs_for_this_thread(),
        generate_uuid(),
        query_job_t(current_microtime(), peer, &job_interruptor));

    int64_t token = q->token();
    use_json_t use_json = q->accepts_r_json() ? use_json_t::YES : use_json_t::NO;

    wait_any_t combined_interruptor(interruptor, &job_interruptor);

    switch (q->type()) {
    case Query_QueryType_START: {
        const profile_bool_t profile = profile_bool_optarg(q);
        const scoped_ptr_t<profile::trace_t> trace = maybe_make_profile_trace(profile);
        env_t env(ctx, &combined_interruptor, global_optargs(q), trace.get_or_null());

        counted_t<const term_t> root_term;
        try {
            Term *t = q->mutable_query();
            compile_env_t compile_env((var_visibility_t()));
            root_term = compile_term(&compile_env, q.make_child(t));
        } catch (const exc_t &e) {
            fill_error(res, Response::COMPILE_ERROR, e.what(), e.backtrace());
            return;
        } catch (const datum_exc_t &e) {
            fill_error(res, Response::COMPILE_ERROR, e.what(), backtrace_t());
            return;
        }

        try {
            rcheck_toplevel(!stream_cache->contains(token),
                            base_exc_t::GENERIC,
                            strprintf("ERROR: duplicate token %" PRIi64, token));
        } catch (const exc_t &e) {
            fill_error(res, Response::CLIENT_ERROR, e.what(), e.backtrace());
            return;
        } catch (const datum_exc_t &e) {
            fill_error(res, Response::CLIENT_ERROR, e.what(), backtrace_t());
            return;
        }

        try {
            scope_env_t scope_env(&env, var_scope_t());
            scoped_ptr_t<val_t> val = root_term->eval(&scope_env);
            if (val->get_type().is_convertible(val_t::type_t::DATUM)) {
                res->set_type(Response::SUCCESS_ATOM);
                datum_t d = val->as_datum();
                d.write_to_protobuf(res->add_response(), use_json);
                if (trace.has()) {
                    trace->as_datum().write_to_protobuf(
                        res->mutable_profile(), use_json);
                }
            } else if (counted_t<grouped_data_t> gd
                       = val->maybe_as_promiscuous_grouped_data(scope_env.env)) {
                res->set_type(Response::SUCCESS_ATOM);
                datum_t d = to_datum_for_client_serialization(std::move(*gd),
                                                              env.reql_version(),
                                                              env.limits());
                d.write_to_protobuf(res->add_response(), use_json);
                if (env.trace != nullptr) {
                    env.trace->as_datum().write_to_protobuf(
                        res->mutable_profile(), use_json);
                }
            } else if (val->get_type().is_convertible(val_t::type_t::SEQUENCE)) {
                counted_t<datum_stream_t> seq = val->as_seq(&env);
                const datum_t arr = seq->as_array(&env);
                if (arr.has()) {
                    res->set_type(Response::SUCCESS_ATOM);
                    arr.write_to_protobuf(res->add_response(), use_json);
                    if (trace.has()) {
                        trace->as_datum().write_to_protobuf(
                            res->mutable_profile(), use_json);
                    }
                } else {
                    stream_cache->insert(token,
                                         use_json,
                                         env.get_all_optargs(),
                                         profile,
                                         seq);
                    bool b = stream_cache->serve(token, res, &combined_interruptor);
                    r_sanity_check(b);
                }
            } else {
                rfail_toplevel(base_exc_t::GENERIC,
                               "Query result must be of type "
                               "DATUM, GROUPED_DATA, or STREAM (got %s).",
                               val->get_type().name());
            }
        } catch (const exc_t &e) {
            fill_error(res, Response::RUNTIME_ERROR, e.what(), e.backtrace());
            return;
        } catch (const datum_exc_t &e) {
            fill_error(res, Response::RUNTIME_ERROR, e.what(), backtrace_t());
            return;
        } catch (const interrupted_exc_t &e) {
            fill_error(res, Response::RUNTIME_ERROR,
                job_interruptor.is_pulsed()
                    ? "Query interrupted through the `rethinkdb.jobs` table."
                    : "Query interrupted.  Did you shut down the server?");
        }
    } break;
    case Query_QueryType_CONTINUE: {
        try {
            bool b = stream_cache->serve(token, res, &combined_interruptor);
            if (!b) {
                auto err = strprintf("Token %" PRIi64 " not in stream cache.", token);
                fill_error(res, Response::CLIENT_ERROR, err, backtrace_t());
            }
        } catch (const exc_t &e) {
            fill_error(res, Response::RUNTIME_ERROR, e.what(), e.backtrace());
            return;
        } catch (const datum_exc_t &e) {
            fill_error(res, Response::RUNTIME_ERROR, e.what(), backtrace_t());
            return;
        } catch (const interrupted_exc_t &e) {
            fill_error(res, Response::RUNTIME_ERROR,
                job_interruptor.is_pulsed()
                    ? "Query interrupted through the `rethinkdb.jobs` table."
                    : "Query interrupted.  Did you shut down the server?");
        }
    } break;
    case Query_QueryType_STOP: {
        try {
            rcheck_toplevel(stream_cache->contains(token), base_exc_t::GENERIC,
                            strprintf("Token %" PRIi64 " not in stream cache.", token));
            stream_cache->erase(token);
            res->set_type(Response::SUCCESS_SEQUENCE);
        } catch (const exc_t &e) {
            fill_error(res, Response::CLIENT_ERROR, e.what(), e.backtrace());
            return;
        }
    } break;
    case Query_QueryType_NOREPLY_WAIT: {
        try {
            rcheck_toplevel(!stream_cache->contains(token),
                            base_exc_t::GENERIC,
                            strprintf("ERROR: duplicate token %" PRIi64, token));
        } catch (const exc_t &e) {
            fill_error(res, Response::CLIENT_ERROR, e.what(), e.backtrace());
            return;
        } catch (const datum_exc_t &e) {
            fill_error(res, Response::CLIENT_ERROR, e.what(), backtrace_t());
            return;
        }

        // NOREPLY_WAIT is just a no-op.
        // This works because we only evaluate one Query at a time
        // on the connection level. Once we get to the NOREPLY_WAIT Query
        // we know that all previous Queries have completed processing.

        // Send back a WAIT_COMPLETE response.
        res->set_type(Response::WAIT_COMPLETE);
    } break;
    default: unreachable();
    }
}

runtime_term_t::runtime_term_t(protob_t<const Backtrace> bt)
    : pb_rcheckable_t(std::move(bt)) { }

runtime_term_t::~runtime_term_t() { }

term_t::term_t(protob_t<const Term> _src)
    : runtime_term_t(get_backtrace(_src)), src(_src) { }
term_t::~term_t() { }

// Uncomment the define to enable instrumentation (you'll be able to see where
// you are in query execution when something goes wrong).
// #define INSTRUMENT 1

#ifdef INSTRUMENT
TLS_with_init(int, DBG_depth, 0);
#define DBG(s, args...) do {                                            \
        std::string DBG_s = "";                                         \
        for (int DBG_i = 0; DBG_i < TLS_get_DBG_depth(); ++DBG_i) DBG_s += " ";   \
        debugf("%s" s, DBG_s.c_str(), ##args);                          \
    } while (0)
#define INC_DEPTH do { TLS_set_DBG_depth(TLS_get_DBG_depth()+1); } while (0)
#define DEC_DEPTH do { TLS_set_DBG_depth(TLS_get_DBG_depth()-1); } while (0)
#else // INSTRUMENT
#define DBG(s, args...)
#define INC_DEPTH
#define DEC_DEPTH
#endif // INSTRUMENT

protob_t<const Term> term_t::get_src() const {
    return src;
}

void term_t::prop_bt(Term *t) const {
    propagate_backtrace(t, &get_src()->GetExtension(ql2::extension::backtrace));
}

scoped_ptr_t<val_t> runtime_term_t::eval(scope_env_t *env, eval_flags_t eval_flags) const {
    // This is basically a hook for unit tests to change things mid-query
    profile::starter_t starter(strprintf("Evaluating %s.", name()), env->env->trace);
    DEBUG_ONLY_CODE(env->env->do_eval_callback());
    DBG("EVALUATING %s (%d):\n", name(), is_deterministic());
    if (env->env->interruptor->is_pulsed()) {
        throw interrupted_exc_t();
    }
    env->env->maybe_yield();
    INC_DEPTH;

#ifdef INSTRUMENT
    try {
#endif // INSTRUMENT
        try {
            scoped_ptr_t<val_t> ret = term_eval(env, eval_flags);
            DEC_DEPTH;
            DBG("%s returned %s\n", name(), ret->print().c_str());
            return ret;
        } catch (const datum_exc_t &e) {
            DEC_DEPTH;
            DBG("%s THREW\n", name());
            rfail(e.get_type(), "%s", e.what());
        }
#ifdef INSTRUMENT
    } catch (...) {
        DEC_DEPTH;
        DBG("%s THREW OUTER\n", name());
        throw;
    }
#endif // INSTRUMENT
}

} // namespace ql
