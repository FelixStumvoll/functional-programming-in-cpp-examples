package examples.json

case class JsonObject(elements: Map[String, Json]):
  def add(key: String, value: Json): JsonObject =
    JsonObject(elements + (key -> value))

enum Json:
  case Null
  case Bool(val value: Boolean)
  case Number(val value: Double)
  case JsonString(val value: String)
  case Object(jsonObject: JsonObject)
  case Array(val elements: List[Json])

def stringify(json: Json): String =
  json match
    case Json.Null => "null"
    case Json.Bool(value) => value.toString
    case Json.Number(value) => value.toString
    case Json.JsonString(value) => s""""$value""""
    case Json.Object(JsonObject(elements)) if elements.isEmpty => "{}"
    case Json.Object(JsonObject(elements)) =>
      val elementsStr = elements.map { case (key, value) =>
        s""""$key": ${stringify(value)}"""
      }.mkString(",")
      s"{$elementsStr}"
    case Json.Array(Nil) => "[]"
    case Json.Array(elements) =>
      val elementsStr = elements.map(stringify).mkString(",")
      s"[$elementsStr]"