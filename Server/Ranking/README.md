# SKYLINE RUSH online ranking

Cloudflare Worker and D1. Korean leaderboard and deterministic input replay verification for runner rules v6 (three lives, 2.5-second recovery protection and 1.2-second hit slowdown). Upload is opt-in. Public data: nickname, score and distance. Anonymous player tokens are hashed on the server. QA records use a separate board.

Build: npm ci, npm run db:generate, npm run build, npm start. Apply generated drizzle SQL to local D1 before tests. node tests/verify.mjs checks C++ fixtures and HTTP behavior at localhost:8787 (public QA writes require explicit authorization).

Online submissions: max 20 minutes and 10,000 inputs. Verification rejects impossible replays and modified scores; it does not attest human input. Device profiles are anonymous; cross-device recovery and platform accounts are not implemented.
