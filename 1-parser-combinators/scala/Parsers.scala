def op: Parser[Int => Int => Int] =
  Parser.char('+').map(_ => (a: Int, b: Int) => a + b)
    .orElse(Parser.char('-').map(_ => _ - _))
    .orElse(Parser.char('*').map(_ => _ * _))
    .map(_.curried)

def calcParser: Parser[Int] =
  Parser.token(Parser.integer).flatMap{ a =>
    Parser.token(op).ap(Parser.pure(a)).ap(Parser.token(Parser.integer))
  }