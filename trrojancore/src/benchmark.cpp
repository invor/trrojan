/// <copyright file="benchmark.cpp" company="Visualisierungsinstitut der Universit�t Stuttgart">
/// Copyright � 2016 - 2018 Visualisierungsinstitut der Universit�t Stuttgart. Alle Rechte vorbehalten.
/// Licensed under the MIT licence. See LICENCE.txt file in the project root for full licence information.
/// </copyright>
/// <author>Valentin Bruder</author>
/// <author>Christoph M�ller</author>

#include "trrojan/benchmark.h"

#include <algorithm>
#include <cassert>
#include <sstream>
#include <stdexcept>

#include "trrojan/log.h"
#include "trrojan/system_factors.h"



/*
 * trrojan::benchmark_base::check_consistency
 */
void trrojan::benchmark_base::check_consistency(const result_set& rs) {
    if (rs.size() > 1) {
        /* Only a result set with more than one element can be inconsistent. */
        auto& reference = rs.front();
        if (reference == nullptr) {
            throw std::invalid_argument("A result_set must not contain nullptr "
                "elements.");
        }

        for (size_t i = 1; i < rs.size(); ++i) {
            auto& element = rs[i];
            if (element == nullptr) {
                throw std::invalid_argument("A result_set must not contain nullptr "
                    "elements.");
            }
            reference->check_consistency(*element);
        }
    }
}


/*
 * trrojan::benchmark_base::merge_results
 */
void trrojan::benchmark_base::merge_results(result_set& l,
        const result_set& r) {
    l.reserve(l.size() + r.size());
    l.insert(l.end(), r.cbegin(), r.cend());
}


/*
 * trrojan::benchmark_base::merge_results
 */
void trrojan::benchmark_base::merge_results(result_set& l, result_set&& r) {
    l.reserve(l.size() + r.size());
    for (auto& e : r) {
        l.push_back(std::move(e));
    }
}


/*
 * trrojan::benchmark_base::factor_device
 */
const std::string trrojan::benchmark_base::factor_device("device");


/*
 * trrojan::benchmark_base::factor_environment
 */
const std::string trrojan::benchmark_base::factor_environment("environment");


/*
 * trrojan::benchmark_base::~benchmark_base
 */
trrojan::benchmark_base::~benchmark_base(void) { }


/*
 * trrojan::benchmark_base::can_run
 */
bool trrojan::benchmark_base::can_run(environment env,
        device device) const {
    return true;
}


/*
 * trrojan::benchmark_base::optimise_order
 */
void trrojan::benchmark_base::optimise_order(configuration_set& inOutConfs) { }


/*
 * trrojan::benchmark_base::required_factors
 */
std::vector<std::string> trrojan::benchmark_base::required_factors(void) const {
    std::vector<std::string> retval;

    for (auto& f : this->_default_configs.factors()) {
        if (f.size() == 0) {
            retval.push_back(f.name());
        }
    }

    return std::move(retval);
}


/*
 * trrojan::benchmark_base::run
 */
size_t trrojan::benchmark_base::run(const configuration_set& configs,
        const on_result_callback& resultCallback,
        const cool_down& coolDown) {
    // Check that caller has provided all required factors.
    this->check_required_factors(configs);

    // Merge missing factors from default configuration.
    auto c = configs;
    c.merge(this->_default_configs, false);

    // Invoke each configuration.
    cool_down_evaluator cde(coolDown);
    size_t retval = 0;
    c.foreach_configuration([&](configuration& c) -> bool {
        try {
            auto e = c.get<trrojan::environment>(environment_base::factor_name);
            auto d = c.get<trrojan::device>(device_base::factor_name);

            cde.check();

            if (this->can_run(e, d)) {
                c.add_system_factors();
                this->log_run(c);
                auto r = resultCallback(std::move(this->run(c)));
                ++retval;
                return r;
            } else {
                log::instance().write_line(log_level::information, "A "
                    "benchmark cannot run with the specified combination of "
                    "environment and device. Skipping it ...");
                return true;
            }

        } catch (const std::exception& ex) {
            log::instance().write_line(ex);
            return false;
        }
    });

    return retval;
}


/*
 * trrojan::benchmark_base::contains
 */
bool trrojan::benchmark_base::contains(const std::vector<std::string>& haystack,
        const std::string& needle) {
    auto it = std::find(haystack.begin(), haystack.end(), needle);
    return (it != haystack.end());
}


/*
 * trrojan::benchmark_base::merge_system_factors
 */
trrojan::configuration& trrojan::benchmark_base::merge_system_factors(
        trrojan::configuration& c) {
    c.add_system_factors();
    return c;
}


/*
 * trrojan::benchmark_base::check_required_factors
 */
void trrojan::benchmark_base::check_required_factors(
        const trrojan::configuration_set& cs) const {
    auto fs = this->required_factors();
    for (auto& f : fs) {
        log::instance().write(log_level::verbose, "Checking availability of "
            "factor \"{}\" in the given configuration ...\n", f.c_str());
        if ((f.size() == 0) && !cs.contains_factor(f)) {
            std::stringstream msg;
            msg << "The given configuration_set does not contain the required "
                "factor \"" << f << "\"." << std::ends;
            throw std::invalid_argument(msg.str());
        }
    }
}


/*
 * trrojan::benchmark_base::log_run
 */
void trrojan::benchmark_base::log_run(const trrojan::configuration& c) const {
    auto factors = trrojan::to_string(c);
    log::instance().write(log_level::information, "Running \"{}\" with {}\n",
        this->name().c_str(), factors.c_str());
}
