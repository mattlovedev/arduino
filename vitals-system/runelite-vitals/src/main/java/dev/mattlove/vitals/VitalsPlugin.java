package dev.mattlove.vitals;

import com.google.inject.Provides;
import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.nio.charset.StandardCharsets;
import java.util.EnumMap;
import java.util.Map;
import javax.inject.Inject;
import lombok.extern.slf4j.Slf4j;
import net.runelite.api.Client;
import net.runelite.api.GameState;
import net.runelite.api.Skill;
import net.runelite.api.events.GameTick;
import net.runelite.client.config.ConfigManager;
import net.runelite.client.eventbus.Subscribe;
import net.runelite.client.events.ConfigChanged;
import net.runelite.client.plugins.Plugin;
import net.runelite.client.plugins.PluginDescriptor;

@Slf4j
@PluginDescriptor(
	name = "Vitals LED",
	description = "Streams boosted Hitpoints and Prayer over UDP to a serial LED-matrix bridge",
	tags = {"hp", "hitpoints", "prayer", "led", "serial", "hardware", "matrix"}
)
public class VitalsPlugin extends Plugin
{
	/** Skills we stream, mapped to the 2-letter tag the bridge and board expect. */
	private static final Map<Skill, String> TRACKED = new EnumMap<>(Skill.class);
	static
	{
		TRACKED.put(Skill.HITPOINTS, "HP");
		TRACKED.put(Skill.PRAYER, "PR");
	}

	@Inject
	private Client client;

	@Inject
	private VitalsConfig config;

	private DatagramSocket socket;
	private InetAddress address;
	private String resolvedHost;

	private final Map<Skill, Long> lastSent = new EnumMap<>(Skill.class);
	private long lastFlushMs;

	@Override
	protected void startUp() throws Exception
	{
		socket = new DatagramSocket();
		resolveAddress();
		lastSent.clear();
		lastFlushMs = 0L;
		log.debug("Vitals LED started, target {}:{}", config.host(), config.port());
	}

	@Override
	protected void shutDown()
	{
		if (socket != null)
		{
			socket.close();
			socket = null;
		}
		address = null;
		resolvedHost = null;
		lastSent.clear();
	}

	@Subscribe
	public void onGameTick(GameTick tick)
	{
		if (socket == null || address == null || client.getGameState() != GameState.LOGGED_IN)
		{
			return;
		}

		final long keepaliveMs = Math.max(0, config.keepaliveSeconds()) * 1000L;
		final boolean keepaliveDue = keepaliveMs > 0
			&& (System.currentTimeMillis() - lastFlushMs) >= keepaliveMs;

		final StringBuilder batch = new StringBuilder();
		for (Map.Entry<Skill, String> entry : TRACKED.entrySet())
		{
			final Skill skill = entry.getKey();
			final int cur = client.getBoostedSkillLevel(skill);
			final int max = client.getRealSkillLevel(skill);
			final long packed = ((long) cur << 32) | (max & 0xffff_ffffL);

			final Long prev = lastSent.get(skill);
			if (keepaliveDue || prev == null || prev != packed)
			{
				batch.append(entry.getValue()).append(' ')
					.append(cur).append(' ')
					.append(max).append('\n');
				lastSent.put(skill, packed);
			}
		}

		if (batch.length() > 0)
		{
			send(batch.toString());
			lastFlushMs = System.currentTimeMillis();
		}
	}

	@Subscribe
	public void onConfigChanged(ConfigChanged event) throws Exception
	{
		if (!VitalsConfig.GROUP.equals(event.getGroup()))
		{
			return;
		}
		resolveAddress();
		lastSent.clear();   // force a full resend with the new settings
		lastFlushMs = 0L;
	}

	private void resolveAddress() throws Exception
	{
		if (address == null || !config.host().equals(resolvedHost))
		{
			address = InetAddress.getByName(config.host());
			resolvedHost = config.host();
		}
	}

	private void send(String message)
	{
		try
		{
			final byte[] bytes = message.getBytes(StandardCharsets.US_ASCII);
			socket.send(new DatagramPacket(bytes, bytes.length, address, config.port()));
		}
		catch (IOException ex)
		{
			log.debug("Vitals LED: UDP send failed", ex);
		}
	}

	@Provides
	VitalsConfig provideConfig(ConfigManager configManager)
	{
		return configManager.getConfig(VitalsConfig.class);
	}
}
