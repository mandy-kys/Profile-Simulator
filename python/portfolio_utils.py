import pandas as pd
import numpy as np

import gurobipy as gp
from gurobipy import GRB


def read_portfolio(filename: str) -> tuple[dict[str, int], dict[str, float]]:
    data = pd.read_csv(
        filename,
        sep=r"\s+",
        header=None,
        names=["Ticker", "Shares", "InitialPrice"]
    )

    initial_shares: dict[str, int] = {
        str(row["Ticker"]): int(row["Shares"])
        for _, row in data.iterrows()
    }

    initial_prices: dict[str, float] = {
        str(row["Ticker"]): float(row["InitialPrice"])
        for _, row in data.iterrows()
    }
    return initial_shares, initial_prices

#(a) Compute the expected return estimates
def estimate_expected_return(prices:pd.DataFrame, t:int, tickers: list[str]) -> np.ndarray:
    
    expected_return = []

    for stock in tickers:
        current_price = prices.loc[t, stock]
        previous_price = prices.loc[t-100, stock]

        mu_hat = (current_price - previous_price)/previous_price
    
        expected_return.append(mu_hat)
        
    return np.array(expected_return)

def rebalance_to_weights(portfolio: Portfolio, target_weights: dict[str, float], current_prices: pd.Series) -> None:

    portfolio_value: float = portfolio.value(current_prices)

    target_shares: dict[str, int] = {}

    #Calculate desired number of whole shares
    for ticker, weight in target_weights.items():

        price: float = float(current_prices[ticker])

        target_dollars: float = portfolio_value * weight

        target_shares[ticker] = int(
            target_dollars / price
        )

    #Sell first
    for ticker in target_weights:

        current_shares: int = portfolio.get_shares(ticker)

        shares_to_sell: int = (
            current_shares - target_shares[ticker]
        )

        if shares_to_sell > 0:
            portfolio.sell_shares(
                ticker,
                shares_to_sell,
                float(current_prices[ticker])
            )

    #Buy second
    for ticker in target_weights:

        current_shares: int = portfolio.get_shares(ticker)

        shares_to_buy: int = (
            target_shares[ticker] - current_shares
        )

        if shares_to_buy > 0:
            portfolio.buy_shares(
                ticker,
                shares_to_buy,
                float(current_prices[ticker])
            )
#Part A
#Solve the linear programming portfolio optimization problem using Gurobi 
def solve_lp(expected_return: np.ndarray, tickers: list[str]) -> dict[str, float]:

    model = gp.Model("LP_Portfolio")
    model.Params.OutputFlag = 0
    
    #Decision variables: stock portfolio weights
    x = model.addVars(
        tickers,
        lb=0.0,
        ub=0.60,
        vtype=GRB.CONTINUOUS,
        name="x"
    )

    #Objective function
    model.setObjective(
        gp.quicksum(
            expected_return[i] * x[tickers[i]]
            for i in range(len(tickers))
        ),
        GRB.MAXIMIZE
    )

    #Total stock investment cannot exceed 100%
    model.addConstr(
        gp.quicksum(x[stock] for stock in tickers) <= 1.0,
        name="budget"
    )

    #NVDA weight cannot exceed AAPL weight by more than 10%
    model.addConstr(
        x["NVDA"] - x["AAPL"] <= 0.10,
        name="NVDA_AAPL"
    )

    #Solve
    model.optimize()

    # Extract optimal stock weights
    optimal_weights = {
        stock: x[stock].X
        for stock in tickers
    }

    return optimal_weights

#Run the LP portfolio strategy over the 1000-period
def run_lp_strategy(
    prices: pd.DataFrame,
    initial_shares: dict[str, int],
    initial_cash: float,
    tickers: list[str]
) -> dict:

    portfolio = Portfolio(
        initial_shares,
        initial_cash
    )

    portfolio_values = []
    stock_weights = []
    cash_weights = []

    for t in range(1, 1001):

        current_prices = prices.loc[t, tickers]

        #Rebalance every 100 periods
        if t % 100 == 0:

            #(a) Compute expected return estimates
            expected_return = estimate_expected_return(
                prices,
                t,
                tickers
            )

            #(b)-(g) Solve LP using Gurobi
            optimal_weights = solve_lp(
                expected_return,
                tickers
            )

            #(h) Compute cash weight
            cash_weight = (
                1.0 - sum(optimal_weights.values())
            )

            #Store model weights
            stock_weights.append({
                "Time": t,
                **optimal_weights
            })

            cash_weights.append({
                "Time": t,
                "Cash": cash_weight
            })

            #(i) Rebalance portfolio
            rebalance_to_weights(
                portfolio,
                optimal_weights,
                current_prices
            )

        # Store portfolio value for every period
        portfolio_values.append(
            portfolio.value(current_prices)
        )

    return {
        "portfolio_values": portfolio_values,
        "stock_weights": stock_weights,
        "cash_weights": cash_weights,
        "buy_count": portfolio.buy_count,
        "sell_count": portfolio.sell_count,
        "dollar_traded": portfolio.dollar_traded
    }

#Part B
#Covariance matrix using the previous 100 one period returns
def estimate_covariance(prices: pd.DataFrame, t: int, tickers: list[str]) -> np.ndarray:
    price_window = prices.loc[t - 100:t, tickers]

    returns = price_window.pct_change().dropna()

    covariance_matrix = returns.cov().to_numpy()

    return covariance_matrix

#Part B
#Solve the QP using Gurobi
def solve_qp(
    expected_return: np.ndarray,
    covariance_matrix: np.ndarray,
    tickers: list[str],
    risk_aversion: float = 10.0
) -> tuple[dict[str, float], float, float]:

    model = gp.Model("QP_Portfolio")
    model.Params.OutputFlag = 0

    #Decision variables
    x = model.addVars(
        tickers,
        lb=0.0,
        ub=0.60,
        vtype=GRB.CONTINUOUS,
        name="x"
    )

    #Expected portfolio return
    return_expression = gp.quicksum(
        expected_return[i] * x[tickers[i]]
        for i in range(len(tickers))
    )

    #Portfolio variance
    risk_expression = gp.quicksum(
        covariance_matrix[i, j]
        * x[tickers[i]]
        * x[tickers[j]]
        for i in range(len(tickers))
        for j in range(len(tickers))
    )

    #Objective
    model.setObjective(
        return_expression - risk_aversion * risk_expression,
        GRB.MAXIMIZE
    )

    #Portfolio constraints
    model.addConstr(
        gp.quicksum(
            x[stock] for stock in tickers
        ) <= 1.0,
        name="budget"
    )

    model.addConstr(
        x["NVDA"] - x["AAPL"] <= 0.10,
        name="NVDA_AAPL"
    )

    #Solve
    model.optimize()

    #Get optimal weights
    optimal_weights = {
        stock: x[stock].X
        for stock in tickers
    }
    
    #Treat very small numerical values as zero
    optimal_weights = {
        stock: 0.0 if abs(weight) < 1e-6 else weight
        for stock, weight in optimal_weights.items()
    }
    
    #Convert weights to numpy vector
    weight_vector = np.array([
        optimal_weights[stock]
        for stock in tickers
    ])
    
    #Expected portfolio return
    portfolio_expected_return = float(
        expected_return @ weight_vector
    )
    
    #Estimated portfolio variance
    portfolio_variance = float(
        weight_vector
        @ covariance_matrix
        @ weight_vector
    )
    
    return (
        optimal_weights,
        portfolio_expected_return,
        portfolio_variance
    )


#Part B
#Run QP strategy over the 1000-period
def run_qp_strategy(
    prices: pd.DataFrame,
    initial_shares: dict[str, int],
    initial_cash: float,
    tickers: list[str],
    risk_aversion: float = 10.0
) -> dict:

    portfolio = Portfolio(
        initial_shares,
        initial_cash
    )

    portfolio_values = []
    stock_weights = []
    cash_weights = []
    expected_returns = []
    portfolio_variances = []

    for t in range(1, 1001):

        current_prices = prices.loc[t, tickers]

        #Rebalance every 100 periods
        if t % 100 == 0:

            #Compute expected return estimates
            expected_return = estimate_expected_return(
                prices,
                t,
                tickers
            )

            #Compute covariance matrix
            covariance_matrix = estimate_covariance(
                prices,
                t,
                tickers
            )

            #Solve QP using Gurobi
            (
                optimal_weights,
                portfolio_expected_return,
                portfolio_variance
            ) = solve_qp(
                expected_return,
                covariance_matrix,
                tickers,
                risk_aversion
            )

            #Compute cash weight
            cash_weight = (
                1.0 - sum(optimal_weights.values())
            )

            #Treat very small numerical values as zero
            if abs(cash_weight) < 1e-8:
                cash_weight = 0.0

            #Store stock weights
            stock_weights.append({
                "Time": t,
                **optimal_weights
            })

            #Store cash weight
            cash_weights.append({
                "Time": t,
                "Cash": cash_weight
            })

            #Store expected portfolio return
            expected_returns.append({
                "Time": t,
                "Expected Return": portfolio_expected_return
            })

            #Store estimated portfolio variance
            portfolio_variances.append({
                "Time": t,
                "Variance": portfolio_variance
            })

            #Rebalance portfolio
            rebalance_to_weights(
                portfolio,
                optimal_weights,
                current_prices
            )

        #Store portfolio value at every time period
        portfolio_values.append(
            portfolio.value(current_prices)
        )

    return {
        "portfolio_values": portfolio_values,
        "stock_weights": stock_weights,
        "cash_weights": cash_weights,
        "expected_returns": expected_returns,
        "portfolio_variances": portfolio_variances,
        "buy_count": portfolio.buy_count,
        "sell_count": portfolio.sell_count,
        "dollar_traded": portfolio.dollar_traded
    }


#Part C
#Solve the ILP with Gurobi
def solve_ilp(
    expected_return: np.ndarray,
    tickers: list[str]
) -> tuple[dict[str, float], list[str]]:

    model = gp.Model("ILP_Portfolio")
    model.Params.OutputFlag = 0

    #Binary decision variables
    y = model.addVars(
        tickers,
        vtype=GRB.BINARY,
        name="y"
    )

    #Objective: maximize expected portfolio return
    model.setObjective(
        gp.quicksum(
            (1.0 / 3.0)
            * expected_return[i]
            * y[tickers[i]]
            for i in range(len(tickers))
        ),
        GRB.MAXIMIZE
    )

    #Select at most two stocks
    model.addConstr(
        gp.quicksum(
            y[stock] for stock in tickers
        ) <= 2,
        name="asset_limit"
    )

    #Solve
    model.optimize()

    #Convert binary selections to stock weights
    optimal_weights = {
        stock: (
            1.0 / 3.0
            if y[stock].X > 0.5
            else 0.0
        )
        for stock in tickers
    }

    #Record selected stocks
    selected_assets = [
        stock
        for stock in tickers
        if y[stock].X > 0.5
    ]

    return optimal_weights, selected_assets

#Part C
#Run the ILP portfolio strategy over the 1000 period
def run_ilp_strategy(
    prices: pd.DataFrame,
    initial_shares: dict[str, int],
    initial_cash: float,
    tickers: list[str]
) -> dict:

    portfolio = Portfolio(
        initial_shares,
        initial_cash
    )

    portfolio_values = []
    stock_weights = []
    cash_weights = []
    selected_assets = []

    for t in range(1, 1001):

        current_prices = prices.loc[t, tickers]

        #Rebalance every 100 periods
        if t % 100 == 0:

            #Compute expected return
            expected_return = estimate_expected_return(
                prices,
                t,
                tickers
            )

            #Solve ILP
            optimal_weights, selected = solve_ilp(
                expected_return,
                tickers
            )

            #Compute cash weight
            cash_weight = (
                1.0 - sum(optimal_weights.values())
            )

            #Store stock weights
            stock_weights.append({
                "Time": t,
                **optimal_weights
            })

            #Store cash weight
            cash_weights.append({
                "Time": t,
                "Cash": cash_weight
            })

            #Store selected assets
            selected_assets.append({
                "Time": t,
                "Selected Assets": ", ".join(selected)
            })

            #Rebalance portfolio
            rebalance_to_weights(
                portfolio,
                optimal_weights,
                current_prices
            )

        #Store portfolio value at every period
        portfolio_values.append(
            portfolio.value(current_prices)
        )

    return {
        "portfolio_values": portfolio_values,
        "stock_weights": stock_weights,
        "cash_weights": cash_weights,
        "selected_assets": selected_assets,
        "buy_count": portfolio.buy_count,
        "sell_count": portfolio.sell_count,
        "dollar_traded": portfolio.dollar_traded
    }

# Part D
#Solve MILP using Gurobi
def solve_milp(
    expected_return: np.ndarray,
    tickers: list[str]
) -> tuple[dict[str, float], list[str]]:

    model = gp.Model("MILP_Portfolio")
    model.Params.OutputFlag = 0

    #Continuous portfolio-weight variables
    x = model.addVars(
        tickers,
        lb=0.0,
        vtype=GRB.CONTINUOUS,
        name="x"
    )

    #Binary asset-selection variables
    y = model.addVars(
        tickers,
        vtype=GRB.BINARY,
        name="y"
    )

    #Objective
    model.setObjective(
        gp.quicksum(
            expected_return[i] * x[tickers[i]]
            for i in range(len(tickers))
        ),
        GRB.MAXIMIZE
    )

    #Total stock allocation cannot exceed 100%
    model.addConstr(
        gp.quicksum(
            x[stock] for stock in tickers
        ) <= 1.0,
        name="budget"
    )

    #NVDA cannot exceed AAPL by more than 10%
    model.addConstr(
        x["NVDA"] - x["AAPL"] <= 0.10,
        name="NVDA_AAPL"
    )

    #Linking constraints
    for stock in tickers:

        model.addConstr(
            x[stock] >= 0.10 * y[stock],
            name=f"min_{stock}"
        )

        model.addConstr(
            x[stock] <= 0.60 * y[stock],
            name=f"max_{stock}"
        )

    #Select at most two stocks
    model.addConstr(
        gp.quicksum(
            y[stock] for stock in tickers
        ) <= 2,
        name="asset_limit"
    )

    #Solve
    model.optimize()

    # Extract optimal stock weights
    optimal_weights = {
        stock: x[stock].X
        for stock in tickers
    }

    #Record selected stocks
    selected_assets = [
        stock
        for stock in tickers
        if y[stock].X > 0.5
    ]

    return optimal_weights, selected_assets

#Run MILP strategy over the 1000 period simulation
def run_milp_strategy(
    prices: pd.DataFrame,
    initial_shares: dict[str, int],
    initial_cash: float,
    tickers: list[str]
) -> dict:

    portfolio = Portfolio(
        initial_shares,
        initial_cash
    )

    portfolio_values = []
    stock_weights = []
    cash_weights = []
    selected_assets = []

    for t in range(1, 1001):

        current_prices = prices.loc[t, tickers]

        #Rebalance every 100 periods
        if t % 100 == 0:

            # Compute expected return estimates
            expected_return = estimate_expected_return(
                prices,
                t,
                tickers
            )

            #Solve MILP
            optimal_weights, selected = solve_milp(
                expected_return,
                tickers
            )

            #Compute cash weight
            cash_weight = (
                1.0 - sum(optimal_weights.values())
            )

            if abs(cash_weight) < 1e-8:
                cash_weight = 0.0

            #Store stock weights
            stock_weights.append({
                "Time": t,
                **optimal_weights
            })

            #Store cash weight
            cash_weights.append({
                "Time": t,
                "Cash": cash_weight
            })

            #Store selected assets
            selected_assets.append({
                "Time": t,
                "Selected Assets": ", ".join(selected)
            })

            #Rebalance portfolio
            rebalance_to_weights(
                portfolio,
                optimal_weights,
                current_prices
            )

        # tore portfolio value at every period
        portfolio_values.append(
            portfolio.value(current_prices)
        )

    return {
        "portfolio_values": portfolio_values,
        "stock_weights": stock_weights,
        "cash_weights": cash_weights,
        "selected_assets": selected_assets,
        "buy_count": portfolio.buy_count,
        "sell_count": portfolio.sell_count,
        "dollar_traded": portfolio.dollar_traded
    }

#Part E
#Solve MIQP using Gurobi
def solve_miqp(
    expected_return: np.ndarray,
    covariance_matrix: np.ndarray,
    tickers: list[str],
    risk_aversion: float = 10.0
) -> tuple[dict[str, float], list[str], float, float]:

    model = gp.Model("MIQP_Portfolio")
    model.Params.OutputFlag = 0

    #Continuous portfolio-weight variables
    x = model.addVars(
        tickers,
        lb=0.0,
        vtype=GRB.CONTINUOUS,
        name="x"
    )

    #Binary asset-selection variables
    y = model.addVars(
        tickers,
        vtype=GRB.BINARY,
        name="y"
    )

    #Expected portfolio return
    return_expression = gp.quicksum(
        expected_return[i] * x[tickers[i]]
        for i in range(len(tickers))
    )

    #Portfolio variance
    risk_expression = gp.quicksum(
        covariance_matrix[i, j]
        * x[tickers[i]]
        * x[tickers[j]]
        for i in range(len(tickers))
        for j in range(len(tickers))
    )

    #Objective
    model.setObjective(
        return_expression - risk_aversion * risk_expression,
        GRB.MAXIMIZE
    )

    #Budget constraint
    model.addConstr(
        gp.quicksum(
            x[stock] for stock in tickers
        ) <= 1.0,
        name="budget"
    )

    #NVDA cannot exceed AAPL by more than 10%
    model.addConstr(
        x["NVDA"] - x["AAPL"] <= 0.10,
        name="NVDA_AAPL"
    )

    #Linking constraints
    for stock in tickers:

        model.addConstr(
            x[stock] >= 0.10 * y[stock],
            name=f"min_{stock}"
        )

        model.addConstr(
            x[stock] <= 0.60 * y[stock],
            name=f"max_{stock}"
        )

    #Select at most two stocks
    model.addConstr(
        gp.quicksum(
            y[stock] for stock in tickers
        ) <= 2,
        name="asset_limit"
    )

    #Solve
    model.optimize()

    #Extract optimal stock weights
    optimal_weights = {
        stock: x[stock].X
        for stock in tickers
    }

    #Record selected assets
    selected_assets = [
        stock
        for stock in tickers
        if y[stock].X > 0.5
    ]

    #Convert weights to numpy vector
    weight_vector = np.array([
        optimal_weights[stock]
        for stock in tickers
    ])

    #Expected portfolio return
    portfolio_expected_return = float(
        expected_return @ weight_vector
    )

    #Estimated portfolio variance
    portfolio_variance = float(
        weight_vector
        @ covariance_matrix
        @ weight_vector
    )

    return (
        optimal_weights,
        selected_assets,
        portfolio_expected_return,
        portfolio_variance
    )

#Part E
#Run the MIQP strategy over the 1000 period simulation
def run_miqp_strategy(
    prices: pd.DataFrame,
    initial_shares: dict[str, int],
    initial_cash: float,
    tickers: list[str],
    risk_aversion: float = 10.0
) -> dict:

    portfolio = Portfolio(
        initial_shares,
        initial_cash
    )

    portfolio_values = []
    stock_weights = []
    cash_weights = []
    selected_assets = []
    expected_returns = []
    portfolio_variances = []

    for t in range(1, 1001):

        current_prices = prices.loc[t, tickers]

        #Rebalance every 100 periods
        if t % 100 == 0:

            #Compute expected return
            expected_return = estimate_expected_return(
                prices,
                t,
                tickers
            )

            #Compute covariance matrix
            covariance_matrix = estimate_covariance(
                prices,
                t,
                tickers
            )

            #Solve MIQP using Gurobi
            (
                optimal_weights,
                selected,
                portfolio_expected_return,
                portfolio_variance
            ) = solve_miqp(
                expected_return,
                covariance_matrix,
                tickers,
                risk_aversion
            )

            #Compute cash weight
            cash_weight = (
                1.0 - sum(optimal_weights.values())
            )

            if abs(cash_weight) < 1e-8:
                cash_weight = 0.0

            #Store stock weights
            stock_weights.append({
                "Time": t,
                **optimal_weights
            })

            #Store cash weight
            cash_weights.append({
                "Time": t,
                "Cash": cash_weight
            })

            #Store selected assets
            selected_assets.append({
                "Time": t,
                "Selected Assets": ", ".join(selected)
            })

            #Store expected return
            expected_returns.append({
                "Time": t,
                "Expected Return": portfolio_expected_return
            })

            #Store portfolio variance
            portfolio_variances.append({
                "Time": t,
                "Variance": portfolio_variance
            })

            #Rebalance portfolio
            rebalance_to_weights(
                portfolio,
                optimal_weights,
                current_prices
            )

        #Store portfolio value every period
        portfolio_values.append(
            portfolio.value(current_prices)
        )

    return {
        "portfolio_values": portfolio_values,
        "stock_weights": stock_weights,
        "cash_weights": cash_weights,
        "selected_assets": selected_assets,
        "expected_returns": expected_returns,
        "portfolio_variances": portfolio_variances,
        "buy_count": portfolio.buy_count,
        "sell_count": portfolio.sell_count,
        "dollar_traded": portfolio.dollar_traded
    }

class Portfolio:
    
    def __init__(self, initial_shares: dict[str, int], initial_cash: float) -> None:

        self.shares: dict[str, int] = initial_shares.copy()
        self.cash: float = float(initial_cash)

        self.buy_count: int = 0
        self.sell_count: int = 0
        self.dollar_traded: float = 0.0

    #Return current cash balance
    def get_cash(self) -> float:
        
        return self.cash

    #Return number of shares held for a stock
    def get_shares(self, ticker: str) -> int:

        return self.shares[ticker]

    #Calculate the total portfolio value = cash + market value of all stock holdings
    def value(self, current_prices: pd.Series) -> float:
       
        stock_value: float = 0.0

        for ticker in self.shares:
            price: float = float(current_prices[ticker])

            stock_value += self.shares[ticker] * price

        return self.cash + stock_value

    #Buy whole shares of a stock
    def buy_shares(self, ticker: str, shares: int, price: float) -> bool:

        shares = int(shares)
        price = float(price)

        if shares <= 0:
            return False

        if price < 0:
            return False

        cost: float = shares * price

        if cost > self.cash:
            return False

        self.shares[ticker] += shares
        self.cash -= cost

        self.buy_count += 1
        self.dollar_traded += cost

        return True


    #Sell whole shares of a stock
    def sell_shares(self, ticker: str, shares: int, price: float) -> bool:

        shares = int(shares)
        price = float(price)

        if shares <= 0:
            return False

        if shares > self.shares[ticker]:
            return False

        proceeds: float = shares * price

        self.shares[ticker] -= shares
        self.cash += proceeds

        self.sell_count += 1
        self.dollar_traded += proceeds

        return True


#Execute trades needed to reach target whole-share holdings
def trade_to_target_shares(
    portfolio: Portfolio,
    target_shares: dict[str, int],
    current_prices: pd.Series
) -> None:

    #Sell first
    for stock in target_shares:
        trade = target_shares[stock] - portfolio.get_shares(stock)

        if trade < 0:
            portfolio.sell_shares(
                stock,
                -trade,
                float(current_prices[stock])
            )

    #Then buy
    for stock in target_shares:
        trade = target_shares[stock] - portfolio.get_shares(stock)

        if trade > 0:
            portfolio.buy_shares(
                stock,
                trade,
                float(current_prices[stock])
            )

#Homework 4 buy and hold strategy on the fixed price path
def run_hw4_buy_hold(
    prices: pd.DataFrame,
    initial_shares: dict[str, int],
    initial_cash: float,
    tickers: list[str]
) -> dict:

    portfolio = Portfolio(initial_shares, initial_cash)
    portfolio_values = []

    for t in range(1, 1001):
        current_prices = prices.loc[t, tickers]

        portfolio_values.append(
            portfolio.value(current_prices)
        )

    return {
        "portfolio_values": portfolio_values,
        "buy_count": portfolio.buy_count,
        "sell_count": portfolio.sell_count,
        "dollar_traded": portfolio.dollar_traded
    }

#Homework 4 mean-reversion strategy on the fixed price path
def run_hw4_mean_reversion(
    prices: pd.DataFrame,
    initial_shares: dict[str, int],
    initial_prices: dict[str, float],
    initial_cash: float,
    tickers: list[str]
) -> dict:

    portfolio = Portfolio(initial_shares, initial_cash)

    reference_prices = {
        stock: initial_prices[stock]
        for stock in tickers
    }

    portfolio_values = []

    for t in range(1, 1001):

        current_prices = prices.loc[t, tickers]

        for stock in tickers:

            current_price = float(current_prices[stock])

            # Buy 10 shares after a 5% price decrease
            if current_price <= 0.95 * reference_prices[stock]:

                if portfolio.get_cash() >= 10 * current_price:

                    portfolio.buy_shares(
                        stock,
                        10,
                        current_price
                    )

                    reference_prices[stock] = current_price

            # Sell 10 shares after a 10% price increase
            elif current_price >= 1.10 * reference_prices[stock]:

                if portfolio.get_shares(stock) >= 10:

                    portfolio.sell_shares(
                        stock,
                        10,
                        current_price
                    )

                    reference_prices[stock] = current_price

        portfolio_values.append(
            portfolio.value(current_prices)
        )

    return {
        "portfolio_values": portfolio_values,
        "buy_count": portfolio.buy_count,
        "sell_count": portfolio.sell_count,
        "dollar_traded": portfolio.dollar_traded
    }

#Homework 4 fixed-weight strategy on the fixed price path
def run_hw4_fixed_weight(
    prices: pd.DataFrame,
    initial_shares: dict[str, int],
    initial_cash: float,
    tickers: list[str]
) -> dict:

    portfolio = Portfolio(initial_shares, initial_cash)
    portfolio_values = []

    for t in range(1, 1001):

        current_prices = prices.loc[t, tickers]

        if t % 100 == 0:

            V = portfolio.value(current_prices)

            A = np.diag([
                current_prices["NVDA"],
                current_prices["AAPL"],
                current_prices["GOOG"]
            ])

            b = np.array([
                0.40 * V,
                0.30 * V,
                0.30 * V
            ])

            target = np.linalg.solve(A, b)

            target_shares = {
                "NVDA": int(target[0]),
                "AAPL": int(target[1]),
                "GOOG": int(target[2])
            }

            trade_to_target_shares(
                portfolio,
                target_shares,
                current_prices
            )

        portfolio_values.append(
            portfolio.value(current_prices)
        )

    return {
        "portfolio_values": portfolio_values,
        "buy_count": portfolio.buy_count,
        "sell_count": portfolio.sell_count,
        "dollar_traded": portfolio.dollar_traded
    }

#Homework 4 investor-view strategy on the fixed price path
def run_hw4_investor_view(
    prices: pd.DataFrame,
    initial_shares: dict[str, int],
    initial_cash: float,
    tickers: list[str]
) -> dict:

    portfolio = Portfolio(initial_shares, initial_cash)
    portfolio_values = []

    for t in range(1, 1001):

        current_prices = prices.loc[t, tickers]

        if t % 100 == 0:

            V = portfolio.value(current_prices)

            nvda_price = float(current_prices["NVDA"])
            aapl_price = float(current_prices["AAPL"])
            goog_price = float(current_prices["GOOG"])

            A = np.array([
                [nvda_price, aapl_price, goog_price],
                [nvda_price, -aapl_price, -goog_price],
                [0.0, aapl_price, -2.0 * goog_price]
            ])

            b = np.array([
                V,
                0.0,
                0.0
            ])

            target = np.linalg.solve(A, b)

            target_shares = {
                "NVDA": int(target[0]),
                "AAPL": int(target[1]),
                "GOOG": int(target[2])
            }

            trade_to_target_shares(
                portfolio,
                target_shares,
                current_prices
            )

        portfolio_values.append(
            portfolio.value(current_prices)
        )

    return {
        "portfolio_values": portfolio_values,
        "buy_count": portfolio.buy_count,
        "sell_count": portfolio.sell_count,
        "dollar_traded": portfolio.dollar_traded
    }
