#ifndef DATABASEIMPL_HPP_W6NHBHAO
#define DATABASEIMPL_HPP_W6NHBHAO

#include "database_types.hpp"

namespace mlb {
namespace data {
struct DatabaseImpl {

    DatabaseImpl(bool useFakeData);

    Players allPlayers() const { return Players{}; }
    ArticleHeaders articleHeaders() const;
    std::optional<Article> article(std::uint32_t id) const { return Article{}; }
    Schedule schedule(const std::string &) const { return Schedule{}; }
    std::optional<Schedule> schedule(const std::string &,
                                     std::uint16_t week) const {
        return Schedule{};
    }

    std::optional<GameReport> gameReport(id_t) const { return GameReport{}; }

    const bool _fake;
};
} // namespace data
} // namespace mlb

#endif /* end of include guard: DATABASEIMPL_HPP_W6NHBHAO */
