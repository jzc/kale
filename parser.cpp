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

  // consume chars until we hit eof, space, lparen or rparen,
  // accumulating the chars in curr_token
  std::string curr_token{static_cast<char>(curr_char)};
  for (;;) {
    curr_char = is.get();
    if (is.eof()
	|| std::isspace(curr_char)
	|| curr_char == '('
	|| curr_char == ')') {
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


Parser::Parser(std::istream& is)
  : t{is} {}

Object Parser::parse() {
  switch (t.read_token()) {
  case Token::number:
    return Object{t.number_data};
  case Token::symbol:
    return Object{memory.symbol(t.symbol_data)};
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
  auto first = memory.cons(parse(), Constants::nil);
  auto last = first;
  for(;;) {
    switch (t.peek()) {
    case Token::rparen:
      t.read_token();
      return Object{first};
    case Token::dot:
      t.read_token();
      last->cdr = parse();
      if (t.read_token() != Token::rparen) {
	throw std::runtime_error("expected right parentheis");
      }	
      return Object{first};
    default:
      break;
    }
    auto cons = memory.cons(parse(), Constants::nil);
    last->cdr = Object{cons};
    last = cons;
  }    
}
