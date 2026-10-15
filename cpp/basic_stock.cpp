//
//  basic_stock.cpp
//  Basic Stock
//
//  Created by Chrysafis Vogiatzis on 8/25/24.
//

#include "basic_stock.h"
#include <fstream>
#include <vector>
#include <iostream>
#include <random>

Stock::Stock(std::string stock_name, int num_shares, double stock_price) {
    name = stock_name;
    shares = num_shares;
    price = stock_price;
}

double Stock::calculate_value() {
    return shares * price;
}

void Stock::update_price(double new_price) {
    this->price = new_price;
}

void Stock::add_shares(int shares) {
    this->shares += shares;
}

void Stock::sell_shares(int shares) {
    if (shares <= this->shares) {
        this->shares -= shares;
    }
}
double Stock::get_price() const{
    return price;
}
int Stock::get_shares() {
    return shares;
}

std::string Stock::get_name() const{
    return name;
}



std::vector<Stock> read_portfolio(std::string filename) {
    std::vector<Stock> stocks;
    std::ifstream file(filename);
    bool done = false;
    while (!done) {
        std::string name;
        int num;
        double price;
        file >> name >> num >> price;
        if (file.eof())
            done = true;
        else {
            Stock s(name, num, price);
            stocks.push_back(s);
        }
    }
    return stocks;
}

Portfolio::Portfolio(std::vector<Stock> initial_stocks, double initial_cash) {
    stocks = initial_stocks;
    cash = initial_cash;
}

//buy and sell shares
void Portfolio::buy_shares(std::string stock_name, int shares, double price) {
    double cost = shares * price;

    if (shares <= 0 || price < 0 || cost > cash) {
        return;
    }

    for (Stock& stock : stocks) {
        if (stock.get_name() == stock_name) {
            stock.add_shares(shares);
            stock.update_price(price);
            cash -= cost;
            return;
        }
    }

    Stock new_stock(stock_name, shares, price);
    stocks.push_back(new_stock);
    cash -= cost;
}


void Portfolio::sell_shares(std::string stock_name, int shares) {
    if (shares <= 0) {
        return;
    }

    for (Stock& stock : stocks) {
        if (stock.get_name() == stock_name) {

            if (shares <= stock.get_shares()) {
                double money_received = shares * stock.get_price();

                stock.sell_shares(shares);
                cash += money_received;
            }

            return;
        }
    }
}

//Compute total portfolio value
double Portfolio::calculate_value() {
    double total = cash;

    for (Stock& stock : stocks) {
        total += stock.calculate_value();
    }

    return total;
}


//get the amount of cash the portfolio
double Portfolio::get_cash() {
    return cash;
}

//get the number of stocks in the portfolio
int Portfolio::get_stock_num() {
    return stocks.size();
}

//access statistics of the portfolio
void Portfolio::show_statistics() {
    std::cout << "Cash: $" << cash << std::endl;
    std::cout << "Total portfolio value: $"
        << calculate_value() << std::endl;

    std::cout << "Stocks:" << std::endl;

    for (Stock& stock : stocks) {
        std::cout << stock.get_name()
            << ": "
            << stock.get_shares() << " shares, "
            << "price per share: $" << stock.get_price()
            << ", total value: $" << stock.calculate_value()
            << std::endl;
    }
}

void Portfolio::update_stock_price(std::string stock_name, double new_price) {
    for (Stock& stock : stocks) {
        if (stock.get_name() == stock_name) {
            stock.update_price(new_price);
            return;
        }
    }
}

int Portfolio::get_shares(std::string stock_name) {
    for (Stock& stock : stocks) {
        if (stock.get_name() == stock_name) {
            return stock.get_shares();
        }
    }

    return 0;
}

double Portfolio::get_stock_price(const std::string& stock_name) const {

    for (const Stock& stock : stocks) {

        if (stock.get_name() == stock_name) {
            return stock.get_price();
        }
    }

    return 0.0;
}

void simulate_price(Stock& stock, double mu, double sigma) {

    static std::random_device rd;
    static std::mt19937 generator(rd());

    std::normal_distribution<double> normal(0.0, 1.0);

    double Z = normal(generator);

    double r = mu + sigma * Z;

    double new_price = stock.get_price() * (1 + r);

    stock.update_price(new_price);
}
