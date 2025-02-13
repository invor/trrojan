/// <copyright file="factor_range.inl" company="Visualisierungsinstitut der Universit�t Stuttgart">
/// Copyright � 2016 - 2018 Visualisierungsinstitut der Universit�t Stuttgart.
/// Licensed under the MIT licence. See LICENCE.txt file in the project root for full licence information.
/// </copyright>
/// <author>Christoph M�ller</author>


/*
 * trrojan::detail::factor_range<T>::~factor_range
 */
template<class T>
trrojan::detail::factor_range<T>::~factor_range(void) { }


/*
 * trrojan::detail::factor_range<T>::clone
 */
template<class T> inline std::unique_ptr<trrojan::detail::factor_base>
trrojan::detail::factor_range<T>::clone(void) const {
    return std::unique_ptr<factor_base>(new factor_range<T>(this->_name,
        this->begin, this->step_size, this->cnt_steps));
}


/*
 * trrojan::detail::factor_range<T>::size
 */
template<class T>
size_t trrojan::detail::factor_range<T>::size(void) const {
    return this->cnt_steps;
}


/*
 * trrojan::detail::factor_range<T>::operator []
 */
template<class T>
trrojan::variant trrojan::detail::factor_range<T>::operator [](
        const size_t i) const {
    if (i >= this->size()) {
        throw std::range_error("The factor does not contain the requested "
            "manifestation.");
    }

    // TODO: implicit cast here is a problem ...
    return (this->begin + i * this->step_size);
}
