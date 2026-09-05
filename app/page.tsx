export default function Home() {
  return (
    <main className="overhead-app">
      <iframe
        className="overhead-simulator"
        src="/intersection.html"
        title="Sem.01 — simulazione dell'incrocio visto dall'alto"
      />
    </main>
  );
}
