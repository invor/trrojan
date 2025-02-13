/// <copyright file="csv_output.h" company="Visualisierungsinstitut der Universit�t Stuttgart">
/// Copyright � 2016 - 2018 Visualisierungsinstitut der Universit�t Stuttgart.
/// Licensed under the MIT licence. See LICENCE.txt file in the project root for full licence information.
/// </copyright>
/// <author>Christoph M�ller</author>

#pragma once

#include <fstream>

#include "trrojan/csv_output_params.h"
#include "trrojan/output.h"


namespace trrojan {

    /// <summary>
    /// Output handler writing the results to a CSV file.
    /// </summary>
    class TRROJANCORE_API csv_output : public output_base {

    public:

        /// <summary>
        /// Initialises a new instance.
        /// </summary>
        csv_output(void);

        /// <summary>
        /// Finalises the instance.
        /// </summary>
        virtual ~csv_output(void);

        /// <inheritdoc />
        virtual void close(void);

        /// <inheritdoc />
        virtual void open(const output_params& params);

        /// <inheritdoc />
        virtual output_base& operator <<(const basic_result& result);

    private:

        void print(const std::string& str);

        void print(const variant& v);

        inline void print(const named_variant& v) {
            this->print(v.value());
        }

        std::ofstream file;

        bool first_line;

        std::shared_ptr<csv_output_params> params;

    };
}
