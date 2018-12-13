#ifndef NES_UTILITY_HPP
#define NES_UTILITY_HPP

namespace nes
{
    namespace internal
    {
        template<class Task_chain, std::size_t... Ns, class F>
        void for_each_impl(std::index_sequence<Ns...>&&, F&& f)
        {
            (std::forward<F>(f)(std::integral_constant<std::size_t, Ns>{}), ...);
        }

    } // detail


    template<class Task_chain, class F>
    void for_each(Task_chain&& e, F&& f)
    {
        using Ns = std::make_index_sequence<std::decay_t<Task_chain>::size()>;
        internal::for_each_impl<Task_chain>(Ns{}, std::forward<F>(f));
    }
} // nes

#endif //NES_UTILITY_HPP
