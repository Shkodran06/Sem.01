import type { Metadata } from 'next';
import { Geist, Geist_Mono } from 'next/font/google';
import './globals.css';

const geistSans = Geist({
  variable: '--font-geist-sans',
  subsets: ['latin'],
});

const geistMono = Geist_Mono({
  variable: '--font-geist-mono',
  subsets: ['latin'],
});

export const metadata: Metadata = {
  title: 'Sem.01 · LumaCross Control',
  description: 'Simulatore bilingue di una logica semaforica svizzera con veicoli, biciclette, pedoni e controllo fail-safe.',
  openGraph: {
    title: 'Sem.01 · LumaCross Control',
    description: 'Simulazione IT/DE del controllo semaforico multi-sensore con sequenze di sicurezza e fase pedonale esclusiva.',
    images: ['/lumacross-social.png'],
  },
};

export default function RootLayout({
  children,
}: Readonly<{
  children: React.ReactNode;
}>) {
  return (
    <html lang="it">
      <body
        className={`${geistSans.variable} ${geistMono.variable} antialiased`}
      >
        {children}
      </body>
    </html>
  );
}
