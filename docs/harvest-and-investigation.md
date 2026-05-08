# Harvest and Investigation

## Passive Harvest Mode

Harvest mode is for public domain-side intelligence. It collects:

- passive hostnames
- public contact-oriented pages
- same-domain public contact emails

## Passive Sources

Current wired sources:

- `crtsh`
- `rapiddns`
- `wayback`
- `all`

`all` runs the current wired public passive sources and fuses overlaps so repeated host sightings can raise confidence.

## Contact Collection

After host collection, LowHunt also visits contact-oriented public pages such as:

- `/contact`
- `/about`
- `/support`
- `/team`
- `/company`
- `/legal`
- `/privacy`

It then extracts same-domain contact emails when they appear to be useful and public.

## Noise Reduction

LowHunt suppresses noisy candidates such as:

- `noreply`-style addresses
- malformed candidates
- localhost-like values
- asset-like strings
- off-domain addresses

## Confidence Behavior

### Hosts

Host confidence grows when:

- the same host is seen in more than one source
- the host remains valid after normalization and deduplication

### Emails

Email confidence grows when:

- it is found on contact-oriented pages
- it uses contact-like local parts such as `contact@`, `info@`, `support@`, `security@`, or `privacy@`

## Combined Investigation Mode

When usernames and a domain are supplied together, LowHunt performs a combined case workflow.

It will:

1. run the passive harvest
2. run username scanning
3. compare harvested email identifiers against supplied usernames
4. produce a correlation confidence score
5. generate a narrative summary
6. store report bundles

## Example

```sh
lowhunt alice bob -d example.com -b all --intel -o investigation.json --format json
```

## Interpreting Correlation

- strong overlap means profile hits and domain-side public identifiers align
- moderate overlap means there are partial matches or supporting side signals
- no overlap does not prove independence; it only means LowHunt did not confirm a direct link in public data
