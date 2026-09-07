CREATE TABLE `players` (
	`id` text PRIMARY KEY NOT NULL,
	`token_hash` text NOT NULL,
	`nickname` text NOT NULL,
	`created_at` integer NOT NULL
);
--> statement-breakpoint
CREATE UNIQUE INDEX `players_token` ON `players` (`token_hash`);--> statement-breakpoint
CREATE TABLE `request_limits` (
	`key` text PRIMARY KEY NOT NULL,
	`hits` integer NOT NULL,
	`expires` integer NOT NULL
);
--> statement-breakpoint
CREATE TABLE `runs` (
	`id` text PRIMARY KEY NOT NULL,
	`player_id` text NOT NULL,
	`board` text NOT NULL,
	`score` integer NOT NULL,
	`distance` real NOT NULL,
	`ticks` integer NOT NULL,
	`seed` integer NOT NULL,
	`replay_hash` text NOT NULL,
	`created_at` integer NOT NULL,
	FOREIGN KEY (`player_id`) REFERENCES `players`(`id`) ON UPDATE no action ON DELETE no action
);
--> statement-breakpoint
CREATE INDEX `runs_board_score` ON `runs` (`board`,`score`);--> statement-breakpoint
CREATE INDEX `runs_player_board` ON `runs` (`player_id`,`board`);