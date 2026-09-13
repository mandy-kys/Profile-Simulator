//
//  main.cpp
//  Basic Stock
//
//  Created by Chrysafis Vogiatzis on 8/25/24.
//

#include <iostream>
#include "basic_stock.h"
#include "matrix.h"
#include <fstream>
#include <algorithm>

int main(int argc, const char* argv[]) {

    std::vector<Stock> myportfolio = read_portfolio("portfolio.txt");

    //Create portfolios
    Portfolio buy_hold(myportfolio, 100000);
    Portfolio mean_reversion(myportfolio, 100000);
    Portfolio rebalance(myportfolio, 100000);
    Portfolio investor_view(myportfolio, 100000);

    //store portfolio values
    std::vector<double> mean_reversion_values;
    std::vector<double> buy_hold_values;
    std::vector<double> rebalance_values;
    std::vector<double> investor_view_values;


    //mean-reversion setup
    std::vector<double> reference_prices;

    for (Stock& stock : myportfolio) {
        reference_prices.push_back(stock.get_price());
    }
    
    int mr_buy_count = 0;
    int mr_sell_count = 0;
    int mr_nvda_trades = 0;
    int mr_aapl_trades = 0;
    int mr_goog_trades = 0;

    //Rebalance statistics
    int rebalance_count = 0;
    int rebalance_buy_count = 0;
    int rebalance_sell_count = 0;
    double rebalance_dollar_traded = 0.0;

    //Investor view statistics
    int investor_rebalance_count = 0;
    int investor_buy_count = 0;
    int investor_sell_count = 0;
    double investor_dollar_traded = 0.0;

    //Initial portfolio value
    double initial_portfolio_value = mean_reversion.calculate_value();
    std::cout << "Initial portfolio value: $" << initial_portfolio_value << std::endl;


    //Start simulation
    for (int t = 1; t <= 1000; t++) {

        //Simulate new prices
        for (int i = 0; i < myportfolio.size(); i++) {

            Stock& stock = myportfolio[i];


            // Simulate price
            if (stock.get_name() == "NVDA") {
                simulate_price(stock, 0.0008, 0.030);
            }

            else if (stock.get_name() == "AAPL") {
                simulate_price(stock, 0.0004, 0.015);
            }

            else if (stock.get_name() == "GOOG") {
                simulate_price(stock, 0.0005, 0.020);
            }


            double current_price = stock.get_price();

            //Update prices in all portfolios
            buy_hold.update_stock_price(
                stock.get_name(),
                current_price
            );

            mean_reversion.update_stock_price(
                stock.get_name(),
                current_price
            );

            rebalance.update_stock_price(
                stock.get_name(),
                current_price
            );

            investor_view.update_stock_price(
                stock.get_name(),
                current_price
            );

        //MEAN-REVERSION STRATEGY
            //Buy rule
            if (current_price <= 0.95 * reference_prices[i]) {

                if (mean_reversion.get_cash() >= 10 * current_price) {

                    mean_reversion.buy_shares(
                        stock.get_name(),
                        10,
                        current_price
                    );

                    mr_buy_count++;

                    if (stock.get_name() == "NVDA")
                        mr_nvda_trades++;

                    else if (stock.get_name() == "AAPL")
                        mr_aapl_trades++;

                    else if (stock.get_name() == "GOOG")
                        mr_goog_trades++;


                    // Reset reference price after trade
                    reference_prices[i] = current_price;
                }
            }

            // Sell rule
            else if (current_price >= 1.10 * reference_prices[i]) {

                if (mean_reversion.get_shares(stock.get_name()) >= 10) {

                    mean_reversion.sell_shares(
                        stock.get_name(),
                        10
                    );

                    mr_sell_count++;

                    if (stock.get_name() == "NVDA")
                        mr_nvda_trades++;

                    else if (stock.get_name() == "AAPL")
                        mr_aapl_trades++;

                    else if (stock.get_name() == "GOOG")
                        mr_goog_trades++;


                    // Reset reference price after trade
                    reference_prices[i] = current_price;
                }
            }
        }


      //REBALANCE STRATEGY

        if (t % 100 == 0) {

            rebalance_count++;

            // Current total portfolio value
            double V = rebalance.calculate_value();


           //Construct Matrix A
            Matrix A(3, 3);

            A.set_value(0, 0, rebalance.get_stock_price("NVDA"));
            A.set_value(1, 1, rebalance.get_stock_price("AAPL"));
            A.set_value(2, 2, rebalance.get_stock_price("GOOG"));

            //Construct Vector B
            Matrix b(3, 1);

            b.set_value(0, 0, 0.40 * V);
            b.set_value(1, 0, 0.30 * V);
            b.set_value(2, 0, 0.30 * V);
    
            //Solve x = A^(-1)b
            Matrix x = A.inverse().multiply(b);

            // Desired number of shares
            int target_NVDA =
                static_cast<int>(x.get_value(0, 0));

            int target_AAPL =
                static_cast<int>(x.get_value(1, 0));

            int target_GOOG =
                static_cast<int>(x.get_value(2, 0));


            //Determine required trades

            int trade_NVDA =
                target_NVDA - rebalance.get_shares("NVDA");

            int trade_AAPL =
                target_AAPL - rebalance.get_shares("AAPL");

            int trade_GOOG =
                target_GOOG - rebalance.get_shares("GOOG");

            //Execute sells first
            if (trade_NVDA < 0) {

                int shares = -trade_NVDA;

                rebalance.sell_shares("NVDA", shares);

                rebalance_sell_count++;

                rebalance_dollar_traded +=
                    shares * rebalance.get_stock_price("NVDA");
            }


            if (trade_AAPL < 0) {

                int shares = -trade_AAPL;

                rebalance.sell_shares(
                    "AAPL",
                    shares
                );

                rebalance_sell_count++;

                rebalance_dollar_traded +=
                    shares * rebalance.get_stock_price("AAPL");
            }


            if (trade_GOOG < 0) {

                int shares = -trade_GOOG;

                rebalance.sell_shares(
                    "GOOG",
                    shares
                );

                rebalance_sell_count++;

                rebalance_dollar_traded +=
                    shares * rebalance.get_stock_price("GOOG");
            }

            //Execute buys

            if (trade_NVDA > 0) {

                rebalance.buy_shares(
                    "NVDA",
                    trade_NVDA,
                    rebalance.get_stock_price("NVDA")
                );

                rebalance_buy_count++;

                rebalance_dollar_traded +=
                    trade_NVDA * rebalance.get_stock_price("NVDA");
            }


            if (trade_AAPL > 0) {

                rebalance.buy_shares(
                    "AAPL",
                    trade_AAPL,
                    rebalance.get_stock_price("AAPL")
                );

                rebalance_buy_count++;

                rebalance_dollar_traded +=
                    trade_AAPL * rebalance.get_stock_price("AAPL");
            }


            if (trade_GOOG > 0) {

                rebalance.buy_shares(
                    "GOOG",
                    trade_GOOG,
                    rebalance.get_stock_price("GOOG")
                );

                rebalance_buy_count++;

                rebalance_dollar_traded +=
                    trade_GOOG * rebalance.get_stock_price("GOOG");
            }
        }


        //Investor-view strategy
        if (t % 100 == 0) {

            investor_rebalance_count++;

            // Current portfolio value
            double V = investor_view.calculate_value();

            double nvda_price =
                investor_view.get_stock_price("NVDA");

            double aapl_price =
                investor_view.get_stock_price("AAPL");

            double goog_price =
                investor_view.get_stock_price("GOOG");


            // Construct A
            Matrix A(3, 3);

            // Budget constraint
            A.set_value(0, 0, nvda_price);
            A.set_value(0, 1, aapl_price);
            A.set_value(0, 2, goog_price);

            // Investment View 1
            A.set_value(1, 0, nvda_price);
            A.set_value(1, 1, -aapl_price);
            A.set_value(1, 2, -goog_price);

            // Investment View 2
            A.set_value(2, 0, 0.0);
            A.set_value(2, 1, aapl_price);
            A.set_value(2, 2, -2.0 * goog_price);


            // Construct b
            Matrix b(3, 1);

            b.set_value(0, 0, V);
            b.set_value(1, 0, 0.0);
            b.set_value(2, 0, 0.0);


            // Solve x = A^(-1)b
            Matrix x = A.inverse().multiply(b);


            // Desired whole-number shares
            int target_NVDA =
                static_cast<int>(x.get_value(0, 0));

            int target_AAPL =
                static_cast<int>(x.get_value(1, 0));

            int target_GOOG =
                static_cast<int>(x.get_value(2, 0));


            // Determine required trades
            int trade_NVDA =
                target_NVDA - investor_view.get_shares("NVDA");

            int trade_AAPL =
                target_AAPL - investor_view.get_shares("AAPL");

            int trade_GOOG =
                target_GOOG - investor_view.get_shares("GOOG");


            // Sell first
            if (trade_NVDA < 0) {

                int shares = -trade_NVDA;

                investor_view.sell_shares("NVDA", shares);

                investor_sell_count++;

                investor_dollar_traded +=
                    shares * investor_view.get_stock_price("NVDA");
            }

            if (trade_AAPL < 0) {

                int shares = -trade_AAPL;

                investor_view.sell_shares("AAPL", shares);

                investor_sell_count++;

                investor_dollar_traded +=
                    shares * investor_view.get_stock_price("AAPL");
            }

            if (trade_GOOG < 0) {

                int shares = -trade_GOOG;

                investor_view.sell_shares("GOOG", shares);

                investor_sell_count++;

                investor_dollar_traded +=
                    shares * investor_view.get_stock_price("GOOG");
            }


            // Then buy
            if (trade_NVDA > 0) {

                investor_view.buy_shares(
                    "NVDA",
                    trade_NVDA,
                    investor_view.get_stock_price("NVDA")
                );

                investor_buy_count++;

                investor_dollar_traded +=
                    trade_NVDA * investor_view.get_stock_price("NVDA");
            }

            if (trade_AAPL > 0) {

                investor_view.buy_shares(
                    "AAPL",
                    trade_AAPL,
                    investor_view.get_stock_price("AAPL")
                );

                investor_buy_count++;

                investor_dollar_traded +=
                    trade_AAPL * investor_view.get_stock_price("AAPL");
            }

            if (trade_GOOG > 0) {

                investor_view.buy_shares(
                    "GOOG",
                    trade_GOOG,
                    investor_view.get_stock_price("GOOG")
                );

                investor_buy_count++;

                investor_dollar_traded +=
                    trade_GOOG * investor_view.get_stock_price("GOOG");
            }
        }

        buy_hold_values.push_back(
            buy_hold.calculate_value()
        );

        mean_reversion_values.push_back(
            mean_reversion.calculate_value()
        );

        rebalance_values.push_back(
            rebalance.calculate_value()
        );

        investor_view_values.push_back(
            investor_view.calculate_value()
        );

    }
    
    //Output CSV for plot
    std::ofstream output("portfolio_values.csv");

    output << "Time,Buy_and_Hold,Mean_Reversion,Fixed_Weight,Investor_View"
        << std::endl;

    for (int t = 0; t < 1000; t++) {
        output << t + 1 << ","
            << buy_hold_values[t] << ","
            << mean_reversion_values[t] << ","
            << rebalance_values[t] << ","
            << investor_view_values[t]
            << std::endl;
    }

    output.close();

    //Report Statistics
    // Final values
    double buy_hold_final = buy_hold_values.back();
    double mean_reversion_final = mean_reversion_values.back();
    double rebalance_final = rebalance_values.back();
    double investor_view_final = investor_view_values.back();

    // Max / Min
    double buy_hold_max = *std::max_element(
        buy_hold_values.begin(), buy_hold_values.end()
    );

    double buy_hold_min = *std::min_element(
        buy_hold_values.begin(), buy_hold_values.end()
    );

    double mean_reversion_max = *std::max_element(
        mean_reversion_values.begin(), mean_reversion_values.end()
    );

    double mean_reversion_min = *std::min_element(
        mean_reversion_values.begin(), mean_reversion_values.end()
    );

    double rebalance_max = *std::max_element(
        rebalance_values.begin(), rebalance_values.end()
    );

    double rebalance_min = *std::min_element(
        rebalance_values.begin(), rebalance_values.end()
    );

    double investor_view_max = *std::max_element(
        investor_view_values.begin(), investor_view_values.end()
    );

    double investor_view_min = *std::min_element(
        investor_view_values.begin(), investor_view_values.end()
    );
    std::cout << "\n===== Part G Report =====" << std::endl;

    std::cout << "\nInitial portfolio value: $"
        << initial_portfolio_value << std::endl;

    std::cout << "\nFinal portfolio values:" << std::endl;
    std::cout << "Buy-and-Hold: $" << buy_hold_final << std::endl;
    std::cout << "Mean-Reversion: $" << mean_reversion_final << std::endl;
    std::cout << "Fixed-Weight: $" << rebalance_final << std::endl;
    std::cout << "Investor-View: $" << investor_view_final << std::endl;

    std::cout << "\nMaximum / Minimum values:" << std::endl;

    std::cout << "Buy-and-Hold: Max = $" << buy_hold_max
        << ", Min = $" << buy_hold_min << std::endl;

    std::cout << "Mean-Reversion: Max = $" << mean_reversion_max
        << ", Min = $" << mean_reversion_min << std::endl;

    std::cout << "Fixed-Weight: Max = $" << rebalance_max
        << ", Min = $" << rebalance_min << std::endl;

    std::cout << "Investor-View: Max = $" << investor_view_max
        << ", Min = $" << investor_view_min << std::endl;

    std::cout << "\nRebalancing events:" << std::endl;
    std::cout << "Fixed-Weight: " << rebalance_count << std::endl;
    std::cout << "Investor-View: " << investor_rebalance_count << std::endl;

    std::cout << "\nBuy / Sell transactions:" << std::endl;
    std::cout << "Buy-and-Hold: 0 buys, 0 sells" << std::endl;

    std::cout << "Mean-Reversion: "
        << mr_buy_count << " buys, "
        << mr_sell_count << " sells"
        << std::endl;

    std::cout << "Fixed-Weight: "
        << rebalance_buy_count << " buys, "
        << rebalance_sell_count << " sells"
        << std::endl;

    std::cout << "Investor-View: "
        << investor_buy_count << " buys, "
        << investor_sell_count << " sells"
        << std::endl;

    std::cout << "\nDollar amount traded:" << std::endl;

    std::cout << "Fixed-Weight: $"
        << rebalance_dollar_traded << std::endl;

    std::cout << "Investor-View: $"
        << investor_dollar_traded << std::endl;

    int buy_hold_trades = 0;
    int mean_reversion_trades = mr_buy_count + mr_sell_count;
    int rebalance_trades = rebalance_buy_count + rebalance_sell_count;
    int investor_view_trades = investor_buy_count + investor_sell_count;

    std::vector<int> trade_counts = {
        buy_hold_trades,
        mean_reversion_trades,
        rebalance_trades,
        investor_view_trades
    };

    std::vector<double> final_values = {
        buy_hold_final,
        mean_reversion_final,
        rebalance_final,
        investor_view_final
    };

    std::vector<std::string> strategy_names = {
        "Buy-and-Hold",
        "Mean-Reversion",
        "Fixed-Weight",
        "Investor-View"
    };

    int most_trades_index = 0;
    int best_index = 0;
    int worst_index = 0;

    for (int i = 1; i < 4; i++) {

        if (trade_counts[i] > trade_counts[most_trades_index])
            most_trades_index = i;

        if (final_values[i] > final_values[best_index])
            best_index = i;

        if (final_values[i] < final_values[worst_index])
            worst_index = i;
    }

    std::cout << "\nStrategy with most trades: "
        << strategy_names[most_trades_index]
        << std::endl;

    std::cout << "Best-performing strategy: "
        << strategy_names[best_index]
        << std::endl;

    std::cout << "Best strategy outperformed worst by: $"
        << final_values[best_index] - final_values[worst_index]
        << std::endl;
}












// Old hw1 code from here

/*
std::vector<Stock> myportfolio = read_portfolio("portfolio.txt");

//Part C Price Simulation & Part D Trading Strategy

//Buy-and-Hold Benchmark strategy
Portfolio buy_hold(myportfolio, 100000);

//Simple mean-reversion strategy
Portfolio mean_reversion(myportfolio, 100000);

//Reference prices B=P0
std::vector<double> reference_prices;

for (Stock& stock : myportfolio) {
    reference_prices.push_back(stock.get_price());
}

//For Part E, storing portfolio values
std::vector<double> mean_reversion_values;
std::vector<double> buy_hold_values;

//Store data for PartG
int buy_count = 0;
int sell_count = 0;

int nvda_trades = 0;
int aapl_trades = 0;
int goog_trades = 0;

double initial_portfolio_value = mean_reversion.calculate_value();
std::cout << "Initial portfolio value: $"
<< initial_portfolio_value << std::endl;


//Start simulation
for (int t = 0; t < 1000; t++) {

    for (int i = 0; i < myportfolio.size(); i++) {

        Stock& stock = myportfolio[i];

        if (stock.get_name() == "NVDA") {
            simulate_price(stock, 0.0008, 0.030);
        }

        else if (stock.get_name() == "AAPL") {
            simulate_price(stock, 0.0004, 0.015);
        }

        else if (stock.get_name() == "GOOG") {
            simulate_price(stock, 0.0005, 0.020);
        }

        double current_price = stock.get_price();

        // make sure price in the portfolio is updated
        mean_reversion.update_stock_price(
            stock.get_name(),
            current_price
        );

        buy_hold.update_stock_price(
            stock.get_name(),
            current_price
        );

        // Buy rule
        if (current_price <= 0.95 * reference_prices[i]) {
            // make sure we have enough cash
            if (mean_reversion.get_cash() >= 10 * current_price) {
                mean_reversion.buy_shares(stock.get_name(), 10, current_price);

                buy_count++;

                if (stock.get_name() == "NVDA") nvda_trades++;
                else if (stock.get_name() == "AAPL") aapl_trades++;
                else if (stock.get_name() == "GOOG") goog_trades++;

                // reset B after trade
                reference_prices[i] = current_price;
            }
        }

        // Sell rule
        else if (current_price >= 1.10 * reference_prices[i]) {
            // make sure we have at least 10 shares
            if (mean_reversion.get_shares(stock.get_name()) >= 10) {
                mean_reversion.sell_shares(stock.get_name(), 10);

                sell_count++;

                if (stock.get_name() == "NVDA") nvda_trades++;
                else if (stock.get_name() == "AAPL") aapl_trades++;
                else if (stock.get_name() == "GOOG") goog_trades++;

                // reset B after trade
                reference_prices[i] = current_price;
            }
        }
        //Hold rule
    }

    //Part E, store values for each time step
    mean_reversion_values.push_back(mean_reversion.calculate_value());
    buy_hold_values.push_back(buy_hold.calculate_value());

}

//For Part F, creating csv file for import
std::ofstream output("portfolio_values.csv");

output << "Time,Mean_Reversion,Buy_and_Hold" << std::endl;

for (int t = 0; t < 1000; t++) {
    output << t + 1 << ","
        << mean_reversion_values[t] << ","
        << buy_hold_values[t]
        << std::endl;
}

output.close();


//Part D & G, compare final values of the two strategy
std::cout << "Buy_Hold Portfolio:" << std::endl;
buy_hold.show_statistics();

std::cout << "Mean_Reversion Portfolio:" << std::endl;
mean_reversion.show_statistics();

double buy_hold_value = buy_hold.calculate_value();
double mean_reversion_value = mean_reversion.calculate_value();

double max_value = *std::max_element(
    mean_reversion_values.begin(),
    mean_reversion_values.end()
);

double min_value = *std::min_element(
    mean_reversion_values.begin(),
    mean_reversion_values.end()
);

std::cout << "Total buys: " << buy_count << std::endl;
std::cout << "Total sells: " << sell_count << std::endl;

std::cout << "Maximum portfolio value: $" << max_value << std::endl;
std::cout << "Minimum portfolio value: $" << min_value << std::endl;

if (nvda_trades >= aapl_trades && nvda_trades >= goog_trades)
std::cout << "Most traded stock: NVDA" << std::endl;
else if (aapl_trades >= goog_trades)
std::cout << "Most traded stock: AAPL" << std::endl;
else
std::cout << "Most traded stock: GOOG" << std::endl;

if (mean_reversion_value > initial_portfolio_value)
std::cout << "Trading strategy was profitable." << std::endl;
else
std::cout << "Trading strategy was not profitable." << std::endl;

if (mean_reversion_value > buy_hold_value)
std::cout << "Mean-reversion performed better by $"
<< mean_reversion_value - buy_hold_value << std::endl;
else if (buy_hold_value > mean_reversion_value)
std::cout << "Buy-and-hold performed better by $"
<< buy_hold_value - mean_reversion_value << std::endl;
else
std::cout << "Both strategies performed equally." << std::endl;

return 0;
*/


/*
The use of <fstream> and <algorithm> packages used AI for understanding the avaiable functions and usage scenarios.
Lines 158 to 183 used AI to generate explanation texts (e.g., "Maximum portfolio value: $", "Mean-reversion performed better by $")
*/