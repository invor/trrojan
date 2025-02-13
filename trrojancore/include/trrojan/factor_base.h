/// <copyright file="factor_base.h" company="Visualisierungsinstitut der Universit�t Stuttgart">
/// Copyright � 2016 - 2018 Visualisierungsinstitut der Universit�t Stuttgart.
/// Licensed under the MIT licence. See LICENCE.txt file in the project root for full licence information.
/// </copyright>
/// <author>Christoph M�ller</author>

#pragma once

#include <memory>
#include <string>

#include "trrojan/export.h"
#include "trrojan/variant.h"


namespace trrojan {
namespace detail {

    /// <summary>
    /// Interface of the different implementations of
    /// <see cref="trrojan::factor" />.
    /// </summary>
    /// <remarks>
    /// This class serves as interface for the facade and provides some shared
    /// implementation like handling of the name of the factor.
    /// </remarks>
    class TRROJANCORE_API factor_base {

    public:

        /// <summary>
        /// Finalises the instance.
        /// </summary>
        virtual ~factor_base(void);

        /// <summary>
        /// Create a deep copy of the factor.
        /// </summary>
        /// <returns>A deep copy of the factor.</returns>
        virtual std::unique_ptr<factor_base> clone(void) const = 0;

        /// <summary>
        /// Answers the name of the factor.
        /// </summary>
        inline const std::string& name(void) const {
            return this->_name;
        }

        /// <summary>
        /// Answer the number of different manifestations the factor has.
        /// </summary>
        /// <returns>The number of manifestations.</returns>
        virtual size_t size(void) const = 0;

        /// <summary>
        /// Answer a specific manifestation.
        /// </summary>
        /// <remarks>
        /// The method returns a deep copy to allow implementations generating
        /// factors on-the-fly. If we required a reference to be returned, this
        /// would result in returning a reference to a temporary.
        /// </remarks>
        /// <param name="i"></param>
        /// <returns>The <paramref name="i" />th manifestation.</returns>
        /// <exception cref="std::range_error"></exception>
        virtual trrojan::variant operator [](const size_t i) const = 0;

        /// <summary>
        /// Test for equality.
        /// </summary>
        /// <param name="rhs">The right-hand side operand.</param>
        /// <returns><c>true</c> if this object and <paramref name="rhs" />
        /// are equal, <c>false</c> otherwise.</returns>
        inline bool operator ==(const factor_base& rhs) const {
            return (this->_name == rhs._name);
        }

        /// <summary>
        /// Test for inequality.
        /// </summary>
        /// <param name="rhs">The right-hand side operand.</param>
        /// <returns><c>false</c> if this object and <paramref name="rhs" />
        /// are equal, <c>true</c> otherwise.</returns>
        inline bool operator !=(const factor_base& rhs) const {
            return !(*this == rhs);
        }

    protected:

        /// <summary>
        /// Initialises a new instance.
        /// </summary>
        /// <param name="name">The name of the factor.</param>
        inline factor_base(const std::string& name = std::string())
            : _name(name) { }

        /// <summary>
        /// The name of the factor.
        /// </summary>
        std::string _name;

    };
}
}
