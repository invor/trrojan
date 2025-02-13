/// <copyright file="graphics_benchmark_base.cpp" company="Visualisierungsinstitut der Universit�t Stuttgart">
/// Copyright � 2016 - 2018 Visualisierungsinstitut der Universit�t Stuttgart.
/// Licensed under the MIT licence. See LICENCE.txt file in the project root for full licence information.
/// </copyright>
/// <author>Christoph M�ller</author>

#include "trrojan/graphics_benchmark_base.h"

#include <stdexcept>


#define _GRAPH_BENCH_BASE_DEFINE_FACTOR(f)                                     \
const std::string trrojan::graphics_benchmark_base::factor_##f(#f)

_GRAPH_BENCH_BASE_DEFINE_FACTOR(manoeuvre);
_GRAPH_BENCH_BASE_DEFINE_FACTOR(manoeuvre_step);
_GRAPH_BENCH_BASE_DEFINE_FACTOR(manoeuvre_steps);
_GRAPH_BENCH_BASE_DEFINE_FACTOR(viewport);

#undef _GRAPH_BENCH_BASE_DEFINE_FACTOR


/*
 * trrojan::graphics_benchmark_base::apply_manoeuvre
 */
void trrojan::graphics_benchmark_base::apply_manoeuvre(trrojan::camera& camera,
        const configuration& config, const glm::vec3& bbs,
        const glm::vec3& bbe) {
    auto curStep = config.get<manoeuvre_step_type>(factor_manoeuvre_step);
    auto manoeuvre = config.get<manoeuvre_type>(factor_manoeuvre);
    auto totalSteps = config.get<manoeuvre_step_type>(factor_manoeuvre_steps);

    auto wtf1 = dynamic_cast<perspective_camera *>(&camera);
    auto wtf2 = dynamic_cast<orthographic_camera *>(&camera);

    if (wtf1 != nullptr) {
        wtf1->set_from_maneuver(manoeuvre, bbs, bbe, curStep, totalSteps);
    } else if (wtf2 != nullptr) {
        wtf2->set_from_maneuver(manoeuvre, bbs, bbe, curStep, totalSteps);
    } else {
        throw std::logic_error("Epic WTF!");
    }
}


/*
 * trrojan::graphics_benchmark_base::get_manoeuvre
 */
void trrojan::graphics_benchmark_base::get_manoeuvre(
        manoeuvre_type& outManoeuvre, manoeuvre_step_type& outCurStep,
        manoeuvre_step_type& outTotalSteps, const configuration& config) {
    outManoeuvre = config.get<manoeuvre_type>(factor_manoeuvre);
    outCurStep = config.get<manoeuvre_step_type>(factor_manoeuvre_step);
    outTotalSteps = config.get<manoeuvre_step_type>(factor_manoeuvre_steps);
}


/*
 * trrojan::graphics_benchmark_base::add_default_manoeuvre
 */
void trrojan::graphics_benchmark_base::add_default_manoeuvre(void) {
    this->_default_configs.add_factor(factor::from_manifestations(
        factor_manoeuvre, manoeuvre_type("diagonal")));
    this->_default_configs.add_factor(factor::from_manifestations(
        factor_manoeuvre_step, static_cast<manoeuvre_step_type>(0)));
    this->_default_configs.add_factor(factor::from_manifestations(
        factor_manoeuvre_steps, static_cast<manoeuvre_step_type>(64)));
}
