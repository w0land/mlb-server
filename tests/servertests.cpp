#include <gtest/gtest.h>
#include <thread>

#include <curlpp/Easy.hpp>
#include <curlpp/Infos.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/cURLpp.hpp>

#include <nlohmann/json.hpp>

#include "log.h"
#include "server.hpp"

#include "database_mock.hpp"

auto get(const std::string &url) {
    using curlpp::infos::ResponseCode;
    curlpp::Easy request;
    request.setOpt<curlpp::options::Url>(url);
    std::ostringstream os;
    curlpp::options::WriteStream ws(&os);
    request.setOpt(ws);
    request.perform();

    return std::make_tuple(ResponseCode::get(request), os.str());
}

struct ServerTestNoDatabase : public ::testing::Test {
    mlb::server::Server s;

    void SetUp() override {
        _serverThread = std::thread([this]() { s.start(); });
    }

    void TearDown() override {
        s.stop();
        _serverThread.join();
    }

    std::thread _serverThread;
};

TEST_F(ServerTestNoDatabase, positive_simple_start_stop) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    s.stop();
}

TEST_F(ServerTestNoDatabase, positive_ping) {
    const auto ret = get("http://localhost:9080/mlb/ping");
    EXPECT_EQ(std::get<0>(ret), 200);
}

TEST_F(ServerTestNoDatabase, negative_get_all_players_no_database_impl_set) {
    curlpp::Easy request;
    request.setOpt<curlpp::options::Url>(
        "http://localhost:9080/mlb/player/all");
    request.perform();
    EXPECT_NE(curlpp::infos::ResponseCode::get(request), 200);
}

struct ServerTest : public ServerTestNoDatabase {
    void SetUp() override {
        s.setDatabaseImpl(db);
        ServerTestNoDatabase::SetUp();
    }

    void TearDown() override { ServerTestNoDatabase::TearDown(); }

    DatabaseMock db;
};

TEST_F(ServerTest, get_all_players) {
    mlb::data::Players players{{"Kenny", "Smith", 100},
                               {"LeBron", "James", 101}};
    EXPECT_CALL(db, allPlayers()).WillOnce(::testing::Return(players));
    const auto ret = get("http://localhost:9080/mlb/player");
    EXPECT_EQ(std::get<0>(ret), 200);
}

TEST_F(ServerTest, get_article_list) {
    mlb::data::ArticleHeaders articles{
        {"Title", "2018-12-10", 10, "Lady Gaga"}};
    EXPECT_CALL(db, articleHeaders()).WillOnce(::testing::Return(articles));
    const auto ret = get("http://localhost:9080/mlb/article");
    EXPECT_EQ(std::get<0>(ret), 200);
    EXPECT_NE(std::get<1>(ret).size(), 0);
}

TEST_F(ServerTest, get_article) {
    mlb::data::Article art{"", {}};
    EXPECT_CALL(db, article(10)).WillOnce(::testing::Return(art));
    const auto ret = get("http://localhost:9080/mlb/article/10");
    EXPECT_EQ(std::get<0>(ret), 200);
}

TEST_F(ServerTest, get_article_not_found) {
    std::optional<mlb::data::Article> opt;
    EXPECT_CALL(db, article(10)).WillOnce(::testing::Return(opt));
    const auto ret = get("http://localhost:9080/mlb/article/10");
    EXPECT_EQ(std::get<0>(ret), 404);
}

TEST_F(ServerTest, get_schedule_major_not_correct) {
    const auto ret = get("http://localhost:9080/mlb/schedule/");
    EXPECT_EQ(std::get<0>(ret), 405);
}

TEST_F(ServerTest, get_schedule_major) {
    mlb::data::Schedule sched;
    EXPECT_CALL(db, schedule("major")).WillOnce(::testing::Return(sched));
    const auto ret = get("http://localhost:9080/mlb/schedule/major");
    EXPECT_EQ(std::get<0>(ret), 200);
}

TEST_F(ServerTest, get_schedule_by_week_not_found) {
    std::optional<mlb::data::Schedule> opt;
    EXPECT_CALL(db, schedule("major", 5)).WillOnce(::testing::Return(opt));
    const auto ret = get("http://localhost:9080/mlb/schedule/major/5");
    EXPECT_EQ(std::get<0>(ret), 404);
}

TEST_F(ServerTest, get_report_incorrect_request) {
    const auto ret = get("http://localhost:9080/mlb/gamereport/");
    EXPECT_EQ(std::get<0>(ret), 406);
}

TEST_F(ServerTest, get_report_by_id) {
    mlb::data::GameReport report{
        10, mlb::data::Team{"", 1}, mlb::data::Team{"", 2}, {}, {}, {}};
    EXPECT_CALL(db, gameReport(5)).WillOnce(::testing::Return(report));
    const auto ret = get("http://localhost:9080/mlb/gamereport/5");
    EXPECT_EQ(std::get<0>(ret), 200);

    auto json = nlohmann::json::parse(std::get<1>(ret));

    EXPECT_TRUE(json.is_object());
}

int main(int argc, char *argv[]) {
    setupLogger();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
