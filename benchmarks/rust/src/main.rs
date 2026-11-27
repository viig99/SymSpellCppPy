use std::env;
use std::error::Error;
use std::fs;
use std::hint::black_box;
use std::path::Path;
use std::time::Instant;

use symspell_rs::{SymSpell, Verbosity};

const CLEAN: &str = "the quick brown fox jumps over the lazy dog";
const COMPOUND: &str = "the quick brown fox jumps overthe lazy dog";
const SEGMENTATION: &str = "thequickbrownfoxjumpsoverthelazydog";
const EDIT_DISTANCE: usize = 2;

fn measure<T>(name: &str, rounds: usize, iterations: usize, mut operation: impl FnMut() -> T) {
    black_box(operation());
    let mut samples = Vec::with_capacity(rounds);
    for _ in 0..rounds {
        let start = Instant::now();
        for _ in 0..iterations {
            black_box(operation());
        }
        samples.push(start.elapsed().as_secs_f64() * 1_000_000.0 / iterations as f64);
    }
    samples.sort_by(f64::total_cmp);
    let median = (samples[(rounds - 1) / 2] + samples[rounds / 2]) / 2.0;
    println!("{name}\t{median:.3}\t{:.3}", samples[0]);
}

fn main() -> Result<(), Box<dyn Error>> {
    let args: Vec<_> = env::args_os().skip(1).collect();
    let [resources, queries_path, rounds] = args.as_slice() else {
        return Err("usage: symspell-rs-benchmark <resources> <queries> <rounds>".into());
    };
    let rounds: usize = rounds
        .to_str()
        .ok_or("rounds must be a positive integer")?
        .parse()?;
    if rounds == 0 {
        return Err("rounds must be positive".into());
    }

    let resources = Path::new(resources);
    let unigrams = resources.join("frequency_dictionary_en_82_765.txt");
    let bigrams = resources.join("frequency_bigramdictionary_en_243_342.txt");
    let query_data = fs::read_to_string(queries_path)?;
    let queries: Vec<_> = query_data
        .lines()
        .filter_map(|line| line.split_whitespace().next())
        .collect();

    let new_index = || SymSpell::new(EDIT_DISTANCE, None, 7, 1);
    let mut spell = new_index();
    spell.load_dictionary(&unigrams, 0, 1, " ")?;
    spell.load_bigram_dictionary(&bigrams, 0, 2, " ")?;
    let lookup = |term, verbosity, transfer_casing| {
        spell.lookup(term, verbosity, EDIT_DISTANCE, &None, None, transfer_casing)
    };
    let compound = || spell.lookup_compound(COMPOUND, EDIT_DISTANCE, &None, false);
    let segment = || spell.word_segmentation(SEGMENTATION, EDIT_DISTANCE);

    assert_eq!(lookup("recieve", Verbosity::Top, false)[0].term, "receive");
    assert_eq!(compound()[0].term, CLEAN);
    assert_eq!(segment().segmented_string, CLEAN);
    assert_eq!(lookup("StreaM", Verbosity::Top, true)[0].term, "StreaM");

    println!(
        "symspell_rs (native Rust), gxhash={}, rounds={rounds}, noisy_queries={}",
        cfg!(feature = "gxhash"),
        queries.len()
    );
    println!("workload\tmedian_us\tmin_us");
    measure("load_unigrams", rounds, 1, || {
        let mut index = new_index();
        index.load_dictionary(&unigrams, 0, 1, " ").unwrap();
        index
    });
    measure("load_bigrams", rounds, 1, || {
        let mut index = new_index();
        index.load_bigram_dictionary(&bigrams, 0, 2, " ").unwrap();
        index
    });
    measure("lookup_exact", rounds, 10_000, || {
        lookup("receive", Verbosity::Top, false)
    });
    measure("lookup_typo", rounds, 2_000, || {
        lookup("recieve", Verbosity::Top, false)
    });
    measure("lookup_casing", rounds, 1_000, || {
        lookup("RecIEve", Verbosity::Top, true)
    });
    measure("lookup_noisy_queries", rounds, 3, || {
        for query in &queries {
            black_box(lookup(query, Verbosity::Closest, false));
        }
    });
    measure("lookup_compound", rounds, 100, compound);
    measure("segment", rounds, 100, segment);
    Ok(())
}
