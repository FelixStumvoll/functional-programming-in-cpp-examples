import scala.concurrent.{ExecutionContext, Future}

def mapReduceParallel[V, Key, MValue, RValue]
(mapFn: V => List[(Key, MValue)], reduceFn: Iterable[MValue] => RValue,
 mapWorkers: Int, reduceWorkers: Int,
 input: Iterable[V])
(using ctx: ExecutionContext): Future[Map[Key, RValue]] =
  val map_futures: Iterable[Future[Iterable[(Key, MValue)]]] = input
    .grouped((input.size / mapWorkers).max(1)).to(Iterable)
    .map(chunk => Future(chunk.flatMap(mapFn)))
  val grouped_future: Future[Map[Key, Iterable[MValue]]] = Future.sequence(map_futures)
    .map(_.flatten)
    .map(_.groupMap(_._1)(_._2))

  grouped_future.flatMap { (grouped: Map[Key, Iterable[MValue]]) =>
    val reduced_values: Iterator[Future[Iterable[(Key, RValue)]]] = grouped.keys
      .grouped((grouped.keys.size / reduceWorkers).max(1))
      .map(keys => Future(keys.map(key => (key, reduceFn(grouped(key))))))
    Future.sequence(reduced_values).map(_.flatten)
  }.map(_.to(Map))