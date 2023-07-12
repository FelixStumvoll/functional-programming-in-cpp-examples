import scala.annotation.tailrec

type ParseResult[A] = Option[(A, String)]

class Parser[A](private val parseFn: String => ParseResult[A]):
  def apply(s: String): ParseResult[A] = parseFn(s)

  def map[B](mapFn: A => B): Parser[B] = Parser.pure(mapFn).ap(this)

/*
  // map implementation using flatMap
  def map[B](mapFn: A => B): Parser[B] = flatMap(a => pure(mapFn(a)))
*/

/*
  // manual map implementation
  def map[B](mapFn: A => B): Parser[B] = Parser(apply(_).map((a, rest) => (mapFn(a), rest)))
*/

  def flatMap[B](mapFn: A => Parser[B]): Parser[B] =
    Parser(apply(_).flatMap { case (a, rest) => mapFn(a)(rest) })

/*
  // flatMap implementation using for comprehension
  def flatMap[B](mapFn: A => Parser[B]): Parser[B] =
    Parser { str =>
      for {
        (a, rest) <- parseFn(str)
        result <- mapFn(a)(rest)
      } yield result
    }
*/

  def filter(predicate: A => Boolean): Parser[A] =
    Parser(apply(_).filter { case (a, _) => predicate(a) })

  def ap[B, C](other: Parser[B])(using ev: A =:= (B => C)): Parser[C] =
    Parser { s =>
      for {
        (fn, rest) <- apply(s)
        (b, bRest) <- other(rest)
        c = ev(fn)(b)
      } yield (c, bRest)
    }

  def ignoreAnd[B](other: Parser[B]): Parser[B] = this.flatMap(_ => other)

  def orElse(other: Parser[A]): Parser[A] = Parser(str => apply(str).orElse(other(str)))

  def many(): Parser[List[A]] = {
    @tailrec
    def many_rec(s: String, soFar: List[A]): (List[A], String) = {
      parseFn(s) match {
        case Some((a, rest)) => many_rec(rest, a :: soFar)
        case None => (soFar.reverse, s)
      }
    }

    Parser(str => Some(many_rec(str, List())))
  }

  def some(): Parser[List[A]] = many().filter(_.nonEmpty)

object Parser:
  def apply[A](parseFn: String => ParseResult[A]): Parser[A] = new Parser(parseFn)

  def pure[A](a: A): Parser[A] = Parser[A](s => Some((a, s)))

  def parseIf(predicate: Char => Boolean): Parser[Char] =
    Parser(s => s.headOption.filter(predicate).map(c => (c, s.tail)))

  def char(char: Char): Parser[Char] = parseIf(_ == char)

  def letter: Parser[Char] = parseIf(_.isLetter)

  def character: Parser[Char] = parseIf(_.isValidChar)

  def digit: Parser[Int] = parseIf(_.isDigit).map(_ - '0')

  def space: Parser[Char] = parseIf(_.isSpaceChar)

  def whitespaces: Parser[String] = space.many().map(_.mkString)

  def token[A](parser: Parser[A]): Parser[A] = whitespaces.ignoreAnd(parser)

  def number: Parser[Int] = digit.some().map(_.fold(0) { (acc, c) => acc * 10 + c })

  def integer: Parser[Int] = char('-').ignoreAnd(number.map(_ * -1)).orElse(number)