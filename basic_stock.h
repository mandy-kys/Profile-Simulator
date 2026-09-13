//
//  basic_stock.h
//  Basic Stock
//
//  Created by Chrysafis Vogiatzis on 8/25/24.
//

#include <string>
#include <vector>

class Stock {
private:
    std::string name;
    int shares;
    double price;
public:
    Stock(std::string stock_name, int num_shares, double stock_price);
    double calculate_value();
    void update_price(double new_price);
    void add_shares(int shares);
    void sell_shares(int shares);
    double get_price()const;
    int get_shares();
    std::string get_name()const;
};

class Portfolio {
private:
    std::vector<Stock> stocks;
    double cash;
public:
    Portfolio(std::vector<Stock> initial_stocks, double initial_cash);

    //getters
    double get_cash();
    int get_stock_num();
    int get_shares(std::string stock_name);

    //buy and sell shares
    void buy_shares(std::string stock_name, int shares, double price);
    void sell_shares(std::string name, int shares);

    //get newest update of stock price
    void update_stock_price(std::string stock_name, double new_price);

    //Compute total portfolio value
    double calculate_value();

    //Access portfolio statistics
    void show_statistics();

    //Access stock price
    double get_stock_price(const std::string& stock_name) const;
};

std::vector<Stock> read_portfolio(std::string filename);

void simulate_price(Stock& stock, double mu, double sigma);
