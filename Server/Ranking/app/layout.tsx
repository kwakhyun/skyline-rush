import type {Metadata} from "next";
import "./globals.css";
export const metadata:Metadata={title:"스카이라인 러시 · 온라인 랭킹",description:"하늘 끝까지 달린 우리의 최고기록. SKYLINE RUSH 온라인 명예의 전당."};
export default function RootLayout({children}:{children:React.ReactNode}){return <html lang="ko"><body>{children}</body></html>;}