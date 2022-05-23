#include "decls.hpp"
  
Tokenizer::Tokenizer(std::istream& is)
  : is{is}
{}

Token Tokenizer::peek() {
  if (!did_peek) {
    last_token = read_token();
    did_peek = true;
  }
  return last_token;
}

Token Tokenizer::read_token() {
  if (did_peek) {
    did_peek = false;
    return last_token;
  }

  decltype(is.get()) curr_char;
  do {
    curr_char = is.get();
  } while (std::isspace(curr_char));
    
  if (is.eof()) { return Token::eof; }
  if (curr_char == '(') { return Token::lparen; }
  if (curr_char == ')') { return Token::rparen; }
  if (curr_char == '\'') { return Token::quote; }

  // consume chars until we hit quote, eof, space, lparen or rparen,
  // accumulating the chars in curr_token
  std::string curr_token{static_cast<char>(curr_char)};
  for (;;) {
    curr_char = is.get();
    if (is.eof()
	|| std::isspace(curr_char)
	|| curr_char == '('
	|| curr_char == ')'
	|| curr_char == '\'') {
      is.unget();
      break;
    }
    curr_token += curr_char;
  }

  if (curr_token == ".") {
    return Token::dot;
  }

  // try converting the token into a number
  // if it succeeds, then we the token we read is a number
  // otherwise, it is a symbol
  bool is_number{false};
  try {
    std::size_t n;      
    number_data = stod(curr_token, &n);
    if (n == curr_token.size()) 
      is_number = true;
  } catch (const std::invalid_argument&) {}
  if (is_number) {
    return Token::number;
  } else {
    symbol_data = std::move(curr_token);
    return Token::symbol;
  }
}


Reader::Reader(std::istream& is)
  : t{is} {}

Object Reader::read() {
  switch (t.read_token()) {
  case Token::number:
    return Object{t.number_data};
  case Token::symbol:
    return Object{memory.symbol(t.symbol_data)};
  case Token::quote:
    return Object{memory.cons(Constants::quote,
			      Object{memory.cons(read(), Constants::nil)})};
  case Token::lparen:
    break;
  case Token::rparen:
    throw std::runtime_error("unmatched parentheis");
  case Token::dot:
    throw std::runtime_error("unexpected dot");
  case Token::eof:
    throw std::runtime_error("reached end of input while parsing");
  }

  switch (t.peek()) {
  case Token::rparen:
    t.read_token();
    // empty list
    return Constants::nil;
  case Token::dot:
    // left paren followed by dot, need at least one expression
    // before a dot can follow
    throw std::runtime_error("unexpected dot");
  default:
    break;
  }

  // parsing a list
  auto first = memory.cons(read(), Constants::nil);
  auto last = first;
  for(;;) {
    switch (t.peek()) {
    case Token::rparen:
      t.read_token();
      return Object{first};
    case Token::dot:
      t.read_token();
      last->cdr = read();
      if (t.read_token() != Token::rparen) {
	throw std::runtime_error("expected right parentheis");
      }	
      return Object{first};
    default:
      break;
    }
    auto cons = memory.cons(read(), Constants::nil);
    last->cdr = Object{cons};
    last = cons;
  }    
}

std::unique_ptr<Form> Parser::parse(const Object& o) {
  if (o.is_number()) {
    return std::make_unique<NumberForm>(o.as_number());
  }

  if (o.is_symbol()) {
    return std::make_unique<SymbolForm>(o.as_symbol());
  }

  auto& car = o.car();
  if (car == Constants::if_) {
    return Parser::parse_if(o);
  }

  if (car == Constants::let) {
    return Parser::parse_let(o);
  }

  if (car == Constants::letrec) {
    return Parser::parse_letrec(o);
  }
  
  if (car == Constants::quote) {
    return Parser::parse_quote(o);
  }

  return Parser::parse_application(o);
}

std::unique_ptr<Form> Parser::parse_if(const Object& o) {
  return std::make_unique<IfForm>(Parser::parse(o.cdr().car()),
				  Parser::parse(o.cdr().cdr().car()),
				  Parser::parse(o.cdr().cdr().cdr().car()));
}

std::unique_ptr<Form> Parser::parse_let(const Object& o) {
  auto& bindings_list = o.cdr().car();
  auto& body = o.cdr().cdr().car();
  std::vector<VariableBinding> bindings;
  for (auto p = bindings_list; p != Constants::nil; p = p.cdr()) {
    auto& binding = p.car();
    auto& binder = binding.car();
    auto& definition = binding.cdr().car();
    bindings.emplace_back(binder.as_symbol(), Parser::parse(definition));
  }
  return std::make_unique<LetForm>(std::move(bindings), parse(body));
}

std::unique_ptr<Form> Parser::parse_letrec(const Object& o) {
  auto& bindings_list = o.cdr().car();
  auto& body = o.cdr().cdr().car();
  std::vector<FunctionBinding> bindings;
  for (auto p = bindings_list; p != Constants::nil; p = p.cdr()) {
    auto& binding = p.car();
    auto& binder = binding.car();
    auto& parameters = binding.cdr().car();
    auto& definition = binding.cdr().cdr().car();
    std::vector<Symbol> parameter_vector;
    for (auto q = parameters; q != Constants::nil; q = q.cdr()) {
      parameter_vector.push_back(q.car().as_symbol());
    }    
    bindings.emplace_back(binder.as_symbol(),
			  std::move(parameter_vector),
			  Parser::parse(definition));
  }
  return std::make_unique<LetrecForm>(std::move(bindings), parse(body));
}

std::unique_ptr<Form> Parser::parse_application(const Object& o) {
  auto function = Parser::parse(o.car());
  std::vector<std::unique_ptr<Form>> argument_vector;
  for (auto p = o.cdr(); p != Constants::nil; p = p.cdr()) {
    argument_vector.emplace_back(Parser::parse(p.car()));
  }
  return std::make_unique<ApplicationForm>(std::move(function),
					   std::move(argument_vector));
}


std::unique_ptr<Form> Parser::parse_quote(const Object& o) {
  auto arg = o.cdr().car();
  return std::make_unique<QuoteForm>(arg);
}
