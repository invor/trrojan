/// <copyright file="problem.inl" company="Visualisierungsinstitut der Universit�t Stuttgart">
/// Copyright � 2016 - 2018 Visualisierungsinstitut der Universit�t Stuttgart.
/// Licensed under the MIT licence. See LICENCE.txt file in the project root for full licence information.
/// </copyright>
/// <author>Christoph M�ller</author>


/*
 * trrojan::stream::problem::allocate
 */
template<trrojan::stream::problem::scalar_type_t T>
void trrojan::stream::problem::allocate(size_t cnt) {
    typedef typename scalar_type_traits<T>::type type;

    if (this->_parallelism < 1) {
        this->_parallelism = 1;
    }
    if (cnt < 1) {
        cnt = 1;
    }

    this->_scalar_size = sizeof(type);

    cnt = cnt * this->_parallelism;
    this->_a.resize(cnt * this->_scalar_size);
    this->_b.resize(cnt * this->_scalar_size);
    this->_c.resize(cnt * this->_scalar_size);

    std::srand(std::time(nullptr));
    std::generate(this->a<type>(), this->a<type>() + cnt, std::rand);
    std::generate(this->b<type>(), this->b<type>() + cnt, std::rand);
}
