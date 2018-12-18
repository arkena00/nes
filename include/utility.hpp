#ifndef NES_UTILITY_HPP
#define NES_UTILITY_HPP

namespace nes
{
    namespace internal
    {
        template<class Tasks, std::size_t... Ns, class F>
        void for_each_impl(std::index_sequence<Ns...>&&, F&& f)
        {
            (std::forward<F>(f)(std::integral_constant<std::size_t, Ns>{}), ...);
        }

    } // detail


    template<class Tasks, class F>
    void for_each(Tasks&& e, F&& f)
    {
        using Ns = std::make_index_sequence<std::tuple_size_v<std::decay_t<Tasks>>>;
        internal::for_each_impl<Tasks>(Ns{}, std::forward<F>(f));
    }
} // nes

#endif //NES_UTILITY_HPP
