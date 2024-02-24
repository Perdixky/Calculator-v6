/*
 * \brief 表达式计算器
 * \author Perdixky
 * \note 本程序的错误分为三种，分别是非法字符错误，表达式错误和运算错误
 * \note 程序中pos的增加在每个函数的末尾，保证在返回时pos指向下一个token
 * \note 程序中对越界的处理是在parseAtom
 * \note 本程序使用了C++23的模块化特性，需要使用C++23编译器
 */
export module calculator;
import std;
import <conio.h>;


class syntax_error final : public std::runtime_error //表达式错误类，用于抛出错误，包括错误信息和错误位置，位置以tokens中的位置为准
{
public:
	explicit syntax_error(const std::string_view message, const std::pair<size_t, size_t> position) : std::runtime_error(message.data()), position_(position) { }
	[[nodiscard]] auto position() const -> std::pair<size_t, size_t>{
		return position_;
	}
private:
	std::pair<size_t, size_t> position_;
};

class operate_error final : public std::runtime_error //运算错误类，用于抛出错误，仅包括错误信息
{
public:
	explicit operate_error(const std::string_view message) : std::runtime_error(message.data()) { }
};

enum class TokenType  //类型枚举
{
	Number, Function, Operator, Parenthesis
};
class Token
{
public:
	Token(const TokenType type, const std::variant<double, std::unordered_map<std::string, std::function<double(double)>>::const_iterator, char>& value, const std::pair<size_t, size_t> position) : type(type), value(value), position(position) { }  //构造函数
	TokenType type;
	std::variant<double, std::unordered_map<std::string, std::function<double(double)>>::const_iterator, char> value;
	std::pair<size_t, size_t> position;
};

/*
 * \brief Token类型的输出函数，目前没用
 */
template<>
struct std::formatter<std::variant<double, std::unordered_map<std::string, std::function<double(double)>>::const_iterator, char>> {
	// parse 函数为空，因为我们不处理任何格式化选项
	static constexpr auto parse(std::format_parse_context& ctx) -> decltype(ctx.begin()) {
		return ctx.begin();
	}

	// format 函数需要处理不同类型的 variant
	template<typename FormatContext>
	auto format(const std::variant<double, std::unordered_map<std::string, std::function<double(double)>>::const_iterator, char>& var, FormatContext& ctx) const -> decltype(ctx.out()) {
		return std::visit([&ctx]<typename T0>(const T0& val) -> decltype(ctx.out()) {
			using T = std::decay_t<T0>;
			if constexpr (std::is_same_v<T, double>) {
				return std::format_to(ctx.out(), "Number: {}", val);
			}
			else if constexpr (std::is_same_v<T, std::unordered_map<std::string, std::function<double(double)>>::const_iterator>) {
				// 假设函数迭代器指向的第一个元素的键是函数名称
				return std::format_to(ctx.out(), "Function: {}", val->first);
			}
			else if constexpr (std::is_same_v<T, char>) {
				return std::format_to(ctx.out(), "Operator or Parenthesis: {}", val);
			}
		}, 
		var);
	}
};

/*
 * \brief 函数映射表，用于存储函数名和函数指针，使用unordered_map，因为unordered_map的查找速度更快
 */
export const std::unordered_map<std::string, std::function<double(double)>> Functions
{
	{"sin",     [](const double arg) { return std::sin(arg); }  },
	{"cos",     [](const double arg) { return std::cos(arg); }  },
	{"tan",     [](const double arg)
	{
		if (std::fmod(arg, std::numbers::pi) == std::numbers::pi / 2)
			throw operate_error("tan函数的参数不能是π/2+kπ！");
		return std::tan(arg);
	}  },
	{"arcsin",  [](const double arg)
	{
		if (arg < -1 || arg > 1)
			throw operate_error("arcsin函数的参数必须在-1到1之间！");
		return std::asin(arg);
	} },
	{"arccos",  [](const double arg)
	{
		if (arg < -1 || arg > 1)
			throw operate_error("arccos函数的参数必须在-1到1之间！");
		return std::acos(arg);
	} },
	{"arctan",  [](const double arg) { return std::atan(arg); } },
	{"sinh",    [](const double arg) { return std::sinh(arg); } },
	{"cosh",    [](const double arg) { return std::cosh(arg); } },
	{"tanh",    [](const double arg) { return std::tanh(arg); } },
	{"arcsinh", [](const double arg) { return std::asinh(arg); }},
	{"arccosh", [](const double arg) { return std::acosh(arg); }},
	{"arctanh", [](const double arg) { return std::atanh(arg); }},
	{"ln",      [](const double arg)
	{
		if (arg < 0)
			throw operate_error("对数函数的参数不能小于0！");
		return std::log(arg);
	}  },
	{"lg",      [](const double arg)
	{
		if (arg < 0)
			throw operate_error("对数函数的参数不能小于0！");
		return std::log10(arg);
	}},
	{"exp",     [](const double arg) { return std::exp(arg); }  },
	{"sqrt",    [](const double arg)
	{
		if (arg < 0)
			throw operate_error("负数不能开平方！");
		return std::sqrt(arg);
	} },
	{"cbrt",    [](const double arg) { return std::cbrt(arg); } },  //立方根
	{"abs",     [](const double arg) { return std::abs(arg); }  },
};

/*
 * \brief 节点类，分为原子节点，一元节点和二元节点，建立共同的基类是为了实现多态
 */
class Node 
{
public:
	Node() = default;  //构造函数
	virtual ~Node() = default;
	[[nodiscard]] virtual auto get_value() const -> double = 0;  //获取值，纯虚函数
	[[nodiscard]] virtual auto is_operator() const -> bool{
		return false;
	};
};

class AtomicNode final : public Node //原子节点类
{
public:
	explicit AtomicNode(const double value) : value_(value) { }  //构造函数
	[[nodiscard]] auto get_value() const -> double override;  //获取值
	
private:
	double value_;  //值
};

class UnaryNode final : public Node  //一元节点类，存储函数或正负号
{
	public:
	UnaryNode(std::unique_ptr<Node> below, const std::function<double(double)>& function) : function_(function), below_(std::move(below)) { }
	[[nodiscard]] auto get_value() const -> double override;  //获取值

private:
	std::function<double(double)> function_;  //函数指针
	std::unique_ptr<Node> below_;  //子节点
};

class BinaryNode final : public Node  //二元节点类
{
public:
	BinaryNode(const char op, std::unique_ptr<Node> left, std::unique_ptr<Node> right) : op_(op), left_(std::move(left)), right_(std::move(right)) {}  //构造函数
	[[nodiscard]] auto get_value() const -> double override;  //获取值

private:
	char op_;  //操作符
	std::unique_ptr<Node> left_;
	std::unique_ptr<Node> right_;  //左右子节点
};

auto AtomicNode::get_value() const -> double  //获取值
{
	return value_;
}
auto UnaryNode::get_value() const -> double  //获取值
{
	return function_(below_->get_value());
}
auto BinaryNode::get_value() const -> double
{
	const double left = left_->get_value();
	const double right = right_->get_value();
	switch (op_)
	{
	case '+':
		return left + right;
	case '-':
		return left - right;
	case '*':
		return left * right;
	case '/':
		if (right == 0)
		{
			throw operate_error("除数不能为0！");
		}
		return left / right;
	case '^':
		if (left == 0 && right < 0)
		{
			throw operate_error("0的负次幂无意义！");
		}
		if(left < 0 && std::floor(right) != right)
		{
			throw operate_error("负数的非整数次幂无意义！");
		}
		return std::pow(left, right);
	default:
		return 0;  //这个return不会被执行，只是为了消除警告
	}
}

auto parseExpression(size_t& pos, const std::vector<Token>& tokens) -> std::unique_ptr<Node>;  //声明，因为parseExpression和parseAtom互相调用，所以需要提前声明
/*
 * \brief 解析原子，即数字和括号内表达式
 */
auto parseAtom(size_t& pos, const std::vector<Token>& tokens) -> std::unique_ptr<Node>
{
	try
	{
		if (tokens.at(pos).type == TokenType::Parenthesis)
		{
			if (std::get<char>(tokens.at(pos).value) == '(')  //使用.at()而不是[]，因为.at()会检查越界并抛出异常
			{
				pos++;
				std::unique_ptr<Node> atom = parseExpression(pos, tokens);
				if (std::get<char>(tokens.at(pos).value) != ')')
				{
					throw syntax_error("缺少右括号！", tokens[pos].position);
				}
				pos++;
				return atom;
			}
		}
		return std::make_unique<AtomicNode>(std::get<double>(tokens.at(pos++).value));  //先使用pos再自增，保证返回时pos指向下一个token
	}
	catch (std::out_of_range&){
		pos--;  //确保不会越界
		throw syntax_error("缺少数字或括号！", tokens[pos].position);
	}
	catch (const std::bad_variant_access&)
	{
		throw syntax_error("此处应为数字！", tokens[pos].position);
	}
}

/*
 * \brief 解析一元节点，即正负号和函数
 */
auto parseFunction(size_t& pos, const std::vector<Token>& tokens) -> std::unique_ptr<Node> {
	while (true) {
		if (pos >= tokens.size()){
			break;  //确保不会越界
		}
		if (tokens[pos].type == TokenType::Operator) {
			if (const char op = std::get<char>(tokens[pos].value); op == '-' || op == '+') {
				pos++;
				std::unique_ptr<Node> below = parseFunction(pos, tokens);  //这里使用递归，因为正负号和函数可以连续出现
				if (op == '-') 
					return std::make_unique<UnaryNode>(std::move(below), [](const double arg) { return -arg; });
				return std::make_unique<UnaryNode>(std::move(below), [](const double arg) { return arg; });
			}
		}
		else if (tokens[pos].type == TokenType::Function) {
			const auto iterator = std::get<std::unordered_map<std::string, std::function<double(double)>>::const_iterator>(tokens[pos].value);
			pos++;
			return std::make_unique<UnaryNode>(std::move(parseFunction(pos, tokens)), iterator->second);
		}
		else
			break; // 退出循环，处理非操作符和非函数的情况
	}
	return parseAtom(pos, tokens);
}

/*
 * \brief 解析因子，即处理指数运算
 */
auto parseFactor(size_t& pos, const std::vector<Token>& tokens) -> std::unique_ptr<Node>
{
	std::unique_ptr<Node> left = std::move(parseFunction(pos, tokens));
	if(pos >= tokens.size()){
		return left;  //确保不会越界
	}
	try{
		while (true) {
			const char op = std::get<char>(tokens[pos].value);
			if (op != '^') {
				break; // 如果不是加号或减号，则跳出循环
			}
			pos++; // 移动到下一个令牌
			if (pos >= tokens.size())
				throw syntax_error("缺少指数！", tokens[--pos].position); //这里必须使用--pos否则会越界
			if (tokens[pos].type == TokenType::Operator)
				throw syntax_error("指数运算符后面应该是数字或括号！", tokens[pos].position);
			std::unique_ptr<Node> right = std::move(parseFunction(pos, tokens));
			left = std::make_unique<BinaryNode>(op, std::move(left), std::move(right)); // 创建二元节点
			if (pos >= tokens.size()) {
				break; // 确保不会越界
			}
		}
	}catch (const std::bad_variant_access&)
	{
		throw syntax_error("表达式结构错误！此处应为运算符", tokens[pos].position);
	}  //try-catch只需要在此处使用一次，因为只要能确保是char类型，就一定是运算符
	return left;
}

/*
 * \brief 解析项，即乘除
 */
auto parseTerm(size_t& pos, const std::vector<Token>& tokens) -> std::unique_ptr<Node>
{
	std::unique_ptr<Node> left = std::move(parseFactor(pos, tokens));
	if (pos >= tokens.size()) {
		return left;  //确保不会越界
	}
	while (true) {
		const char op = std::get<char>(tokens[pos].value);
		if (op != '*' && op != '/') {
			break; // 如果不是加号或减号，则跳出循环
		}
		pos++; // 移动到下一个令牌
		if (pos >= tokens.size())
			throw syntax_error("缺少乘除号后面的因子！", tokens[--pos].position);
		if (tokens[pos].type == TokenType::Operator)
			throw syntax_error("乘除号后面应该是数字或括号！", tokens[pos].position);
		std::unique_ptr<Node> right = std::move(parseFactor(pos, tokens));
		left = std::make_unique<BinaryNode>(op, std::move(left), std::move(right)); // 创建二元节点
		if (pos >= tokens.size()) {
			break; // 确保不会越界
		}
	}
	return left;
}

/*
 * \brief 解析表达式，即加减
 */
auto parseExpression(size_t& pos, const std::vector<Token>& tokens) -> std::unique_ptr<Node>
{
	std::unique_ptr<Node> left = std::move(parseTerm(pos, tokens));
	if (pos >= tokens.size()) {
		return left;  //确保不会越界
	}
	while (true) {
		const char op = std::get<char>(tokens[pos].value);
		if (op != '+' && op != '-') {
			break;  //如果不是加号或减号，则跳出循环
		}
		pos++;  //移动到下一个令牌
		if (pos >= tokens.size())
			throw syntax_error("缺少加减号后面的项！", tokens[--pos].position);
		if (tokens[pos].type == TokenType::Operator)
			throw syntax_error("加减号后面应该是数字或括号！", tokens[pos].position);
		std::unique_ptr<Node> right = std::move(parseTerm(pos, tokens));
		left = std::make_unique<BinaryNode>(op, std::move(left), std::move(right)) ; // 创建二元节点
		if (pos >= tokens.size()) {
			break;  //确保不会越界
		}
	}
	return left;
}

/*
 * \brief 从表达式中获取tokens
 * \param expression 表达式
 */
auto get_tokens(const std::string_view expression) -> std::vector<Token>
{
	std::vector<Token> tokens;
	std::string number;
	std::string function;

	auto function_flush = [&function, &tokens](const size_t i)  //将缓冲区的内容冲洗到tokens中
		{
			if (!function.empty())
			{
				if (auto iterator = Functions.find(function); iterator != Functions.end())
					tokens.emplace_back(TokenType::Function, iterator, std::pair(i - function.size(), i - 1));
				else
					throw syntax_error(std::format("无法处理的函数：{}", function), std::pair(i - function.size(), i - 1));
				function.clear();
			}
		};
	auto number_flush = [&number, &tokens](const size_t i)
		{
			if (!number.empty())
			{
				try
				{
					tokens.emplace_back(TokenType::Number, std::stod(number), std::pair(i - number.size(), i - 1));
					number.clear();
				}
				catch (const std::invalid_argument&) {
					throw syntax_error("无法处理的数字！", std::pair(i - number.size(), i - 1));
				}
			}
		};
	for (size_t i{ 0 }; i < expression.size(); i++)
	{
		if (const char c = expression[i]; std::isdigit(static_cast<unsigned char>(c)) || c == '.') {
			function_flush(i);
			number += c;
		}
		else if (std::isalpha(static_cast<unsigned char>(c))) {
			number_flush(i);
			function += c;
		}
		else if (std::isspace(static_cast<unsigned char>(c)))
		{
			number_flush(i);
			function_flush(i);
		}
		else
		{
			number_flush(i);
			function_flush(i);
			switch (c) {
			case '+':
			case '-':
			case '*':
			case '/':
			case '^':
				tokens.emplace_back(TokenType::Operator, c, std::pair(i, i));
				break;
			case '(':
			case ')':
				tokens.emplace_back(TokenType::Parenthesis, c, std::pair(i, i));
				break;
			default:
				throw syntax_error("非法字符！", std::pair(i, i));
			}
		}
	}
	// 处理表达式末尾的数字或函数名，使得在最后的数字不会被忽略
	number_flush(expression.size());
	function_flush(expression.size());
	return tokens;  // 由于NRVO，不必使用std::move
}

export auto eval(const std::string_view expression) -> double
{
	try {
		size_t pos{ 0 };  // 非常量引用的初始值必须是左值
		return parseExpression(pos, std::forward<std::vector<Token>>(get_tokens(expression)))->get_value();
	}
	catch (syntax_error& e)
	{
		for(size_t i = 0; i < e.position().first; i++)
		{
			std::print(" ");
		}
		for(size_t i = e.position().first; i <= e.position().second; i++)
		{
			std::print("^");
		}
		std::print("\n");
		std::println("{}", e.what());
		return std::nan("1");
	}
	catch (operate_error& e)
	{
		std::println("{}", e.what());
		return std::nan("1");
	}
	catch(...)
	{
		std::println("未知错误！即将推出程序……\n请复制您的表达式并反馈问题");
		std::println("请按任意键继续……");
		_getch();
		return std::nan("1");
	}
}